/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <linux/usb/gadget.h>

#include <soc/qcom/bam_dmux.h>
#include <linux/usb_bam.h>

#include "u_bam_data.h"

#define BAM2BAM_DATA_N_PORTS	1
#define BAM_DATA_RX_Q_SIZE	16
#define BAM_DATA_MUX_RX_REQ_SIZE  2048   /* Must be 1KB aligned */
#define BAM_DATA_PENDING_LIMIT	220

#define SYS_BAM_RX_PKT_FLOW_CTRL_SUPPORT	1
#define SYS_BAM_RX_PKT_FCTRL_EN_TSHOLD		500
#define SYS_BAM_RX_PKT_FCTRL_DIS_TSHOLD		300

static unsigned int bam_ipa_rx_fctrl_support = SYS_BAM_RX_PKT_FLOW_CTRL_SUPPORT;
module_param(bam_ipa_rx_fctrl_support, uint, S_IRUGO | S_IWUSR);

static unsigned int bam_ipa_rx_fctrl_en_thld = SYS_BAM_RX_PKT_FCTRL_EN_TSHOLD;
module_param(bam_ipa_rx_fctrl_en_thld, uint, S_IRUGO | S_IWUSR);

static unsigned int bam_ipa_rx_fctrl_dis_thld = SYS_BAM_RX_PKT_FCTRL_DIS_TSHOLD;
module_param(bam_ipa_rx_fctrl_dis_thld, uint, S_IRUGO | S_IWUSR);

static struct workqueue_struct *bam_data_wq;
static int n_bam2bam_data_ports;

unsigned int bam_data_rx_q_size = BAM_DATA_RX_Q_SIZE;
module_param(bam_data_rx_q_size, uint, S_IRUGO | S_IWUSR);

static unsigned int bam_data_mux_rx_req_size = BAM_DATA_MUX_RX_REQ_SIZE;
module_param(bam_data_mux_rx_req_size, uint, S_IRUGO | S_IWUSR);

#define SPS_PARAMS_SPS_MODE		BIT(5)
#define SPS_PARAMS_TBE		        BIT(6)
#define MSM_VENDOR_ID			BIT(16)

struct rndis_data_ch_info {
	u32 max_transfer_size;
	u32 max_packets_number;
	u32 prod_clnt_hdl;
	u32 cons_clnt_hdl;
	void *priv;
};

struct sys2ipa_sw_data {
	void		*teth_priv;
	ipa_notify_cb	teth_cb;
};

struct bam_data_ch_info {
	unsigned long		flags;
	unsigned		id;

	struct bam_data_port	*port;
	struct work_struct	write_tobam_w;

	struct usb_request	*rx_req;
	struct usb_request	*tx_req;

	u32			src_pipe_idx;
	u32			dst_pipe_idx;
	u8			src_connection_idx;
	u8			dst_connection_idx;
	int			src_bam_idx;
	int			dst_bam_idx;

	enum function_type			func_type;
	enum transport_type			trans;
	struct usb_bam_connect_ipa_params	ipa_params;

	/* UL workaround parameters */
	struct sys2ipa_sw_data	ul_params;
	struct list_head	rx_idle;
	struct sk_buff_head	rx_skb_q;
	struct sk_buff_head	rx_skb_idle;
	enum usb_bam_pipe_type	src_pipe_type;
	enum usb_bam_pipe_type	dst_pipe_type;
	unsigned int		pending_with_bam;
	int			rx_buffer_size;

	unsigned int		rx_flow_control_disable;
	unsigned int		rx_flow_control_enable;
	unsigned int		rx_flow_control_triggered;
};

static struct work_struct *rndis_conn_w;
static struct work_struct *rndis_disconn_w;
static bool is_ipa_rndis_net_on;

struct bam_data_port {
	bool                            is_connected;
	unsigned			port_num;
	spinlock_t			port_lock_ul;
	unsigned int                    ref_count;
	struct data_port		*port_usb;
	struct bam_data_ch_info		data_ch;

	struct work_struct		connect_w;
	struct work_struct		disconnect_w;
	struct work_struct		suspend_w;
	struct work_struct		resume_w;
};
struct  usb_bam_data_connect_info {
	u32 usb_bam_pipe_idx;
	u32 peer_pipe_idx;
	u32 usb_bam_handle;
};

struct bam_data_port *bam2bam_data_ports[BAM2BAM_DATA_N_PORTS];
static struct rndis_data_ch_info rndis_data;

static void bam2bam_data_suspend_work(struct work_struct *w);
static void bam2bam_data_resume_work(struct work_struct *w);
static void bam2bam_data_port_free(int portno);

/*----- sys2bam towards the IPA (UL workaround) --------------- */

static int bam_data_alloc_requests(struct usb_ep *ep, struct list_head *head,
		int num,
		void (*cb)(struct usb_ep *ep, struct usb_request *),
		gfp_t flags)
{
	int i;
	struct usb_request *req;

	pr_debug("%s: ep:%p head:%p num:%d cb:%p", __func__,
			ep, head, num, cb);

	for (i = 0; i < num; i++) {
		req = usb_ep_alloc_request(ep, flags);
		if (!req) {
			pr_err("%s: req allocated:%d\n", __func__, i);
			return list_empty(head) ? -ENOMEM : 0;
		}
		req->complete = cb;
		list_add(&req->list, head);
	}

	return 0;
}

/* This function should be called with port_lock_ul lock taken */
static struct sk_buff *bam_data_alloc_skb_from_pool(
	struct bam_data_port *port)
{
	struct bam_data_ch_info *d;
	struct sk_buff *skb;

	if (!port)
		return NULL;
	d = &port->data_ch;
	if (!d)
		return NULL;

	if (d->rx_skb_idle.qlen == 0) {
		/*
		 * In case skb idle pool is empty, we allow to allocate more
		 * skbs so we dynamically enlarge the pool size when needed.
		 * Therefore, in steady state this dynamic allocation will
		 * stop when the pool will arrive to its optimal size.
		 */
		pr_debug("%s: allocate skb\n", __func__);
		skb = alloc_skb(d->rx_buffer_size + BAM_MUX_HDR, GFP_ATOMIC);
		if (!skb)
			pr_err("%s: alloc skb failed\n", __func__);
		else
			skb_reserve(skb, BAM_MUX_HDR);
	} else {
		pr_debug("%s: pull skb from pool\n", __func__);
		skb = __skb_dequeue(&d->rx_skb_idle);
	}

	return skb;
}

static void bam_data_free_skb_to_pool(
	struct bam_data_port *port,
	struct sk_buff *skb)
{
	struct bam_data_ch_info *d;
	unsigned long flags;

	if (!port) {
		dev_kfree_skb_any(skb);
		return;
	}
	d = &port->data_ch;
	if (!d) {
		dev_kfree_skb_any(skb);
		return;
	}

	spin_lock_irqsave(&port->port_lock_ul, flags);
	skb->len = 0;
	skb_reset_tail_pointer(skb);
	__skb_queue_tail(&d->rx_skb_idle, skb);
	spin_unlock_irqrestore(&port->port_lock_ul, flags);
}

static void bam_data_write_done(void *p, struct sk_buff *skb)
{
	struct bam_data_port	*port = p;
	struct bam_data_ch_info	*d = &port->data_ch;
	unsigned long flags;

	if (!skb)
		return;

	bam_data_free_skb_to_pool(port, skb);

	spin_lock_irqsave(&port->port_lock_ul, flags);
	d->pending_with_bam--;

	pr_debug("%s: port:%p d:%p pbam:%u, pno:%d\n", __func__,
			port, d, d->pending_with_bam, port->port_num);

	spin_unlock_irqrestore(&port->port_lock_ul, flags);

	queue_work(bam_data_wq, &d->write_tobam_w);
}

static void bam_data_ipa_sys2bam_notify_cb(void *priv,
		enum ipa_dp_evt_type event, unsigned long data)
{
	struct sys2ipa_sw_data *ul = (struct sys2ipa_sw_data *)priv;
	struct bam_data_port	*port;
	struct bam_data_ch_info	*d;

	switch (event) {
	case IPA_WRITE_DONE:
		d = container_of(ul, struct bam_data_ch_info, ul_params);
		port = container_of(d, struct bam_data_port, data_ch);
		/* call into bam_demux functionality that'll recycle the data */
		bam_data_write_done(port, (struct sk_buff *)(data));
		break;
	case IPA_RECEIVE:
		/* call the callback given by tethering driver init function
		 * (and was given to ipa_connect)
		 */
		if (ul->teth_cb)
			ul->teth_cb(ul->teth_priv, event, data);
		break;
	default:
		/* unexpected event */
		pr_err("%s: unexpected event %d\n", __func__, event);
		break;
	}
}


static void bam_data_start_rx(struct bam_data_port *port)
{
	struct usb_request		*req;
	struct bam_data_ch_info		*d;
	struct usb_ep			*ep;
	int				ret;
	struct sk_buff			*skb;
	unsigned long			flags;

	spin_lock_irqsave(&port->port_lock_ul, flags);
	if (!port->port_usb) {
		spin_unlock_irqrestore(&port->port_lock_ul, flags);
		return;
	}

	d = &port->data_ch;
	ep = port->port_usb->out;

	while (port->port_usb && !list_empty(&d->rx_idle)) {

		if (bam_ipa_rx_fctrl_support &&
			d->rx_skb_q.qlen >= bam_ipa_rx_fctrl_en_thld)
			break;

		req = list_first_entry(&d->rx_idle, struct usb_request, list);
		skb = bam_data_alloc_skb_from_pool(port);
		if (!skb)
			break;
		list_del(&req->list);
		req->buf = skb->data;
		req->length = d->rx_buffer_size;
		req->context = skb;
		spin_unlock_irqrestore(&port->port_lock_ul, flags);
		ret = usb_ep_queue(ep, req, GFP_ATOMIC);
		spin_lock_irqsave(&port->port_lock_ul, flags);
		if (ret) {
			spin_unlock_irqrestore(&port->port_lock_ul, flags);
			bam_data_free_skb_to_pool(port, skb);
			spin_lock_irqsave(&port->port_lock_ul, flags);


			pr_err("%s: rx queue failed %d\n", __func__, ret);

			if (port->port_usb)
				list_add(&req->list, &d->rx_idle);
			else
				usb_ep_free_request(ep, req);
			break;
		}
	}

	spin_unlock_irqrestore(&port->port_lock_ul, flags);
}

static void bam_data_epout_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct bam_data_port	*port = ep->driver_data;
	struct bam_data_ch_info	*d = &port->data_ch;
	struct sk_buff		*skb = req->context;
	int			status = req->status;
	int			queue = 0;

	switch (status) {
	case 0:
		skb_put(skb, req->actual);
		queue = 1;
		break;
	case -ECONNRESET:
	case -ESHUTDOWN:
		/* cable disconnection */
		bam_data_free_skb_to_pool(port, skb);
		req->buf = 0;
		usb_ep_free_request(ep, req);
		return;
	default:
		pr_err("%s: %s response error %d, %d/%d\n", __func__,
			ep->name, status, req->actual, req->length);
		bam_data_free_skb_to_pool(port, skb);
		break;
	}

	spin_lock(&port->port_lock_ul);
	if (queue) {
		__skb_queue_tail(&d->rx_skb_q, skb);
		if (!usb_bam_get_prod_granted(d->dst_connection_idx)) {
			list_add_tail(&req->list, &d->rx_idle);
			spin_unlock(&port->port_lock_ul);
			return;
		} else
			queue_work(bam_data_wq, &d->write_tobam_w);
	}

	if (bam_mux_rx_fctrl_support &&
		d->rx_skb_q.qlen >= bam_ipa_rx_fctrl_en_thld) {
		if (!d->rx_flow_control_triggered) {
			d->rx_flow_control_triggered = 1;
			d->rx_flow_control_enable++;
		}
		list_add_tail(&req->list, &d->rx_idle);
		spin_unlock(&port->port_lock_ul);
		return;
	}

	skb = bam_data_alloc_skb_from_pool(port);
	spin_unlock(&port->port_lock_ul);
	if (!skb) {
		list_add_tail(&req->list, &d->rx_idle);
		return;
	}
	req->buf = skb->data;
	req->length = d->rx_buffer_size;
	req->context = skb;

	status = usb_ep_queue(ep, req, GFP_ATOMIC);
	if (status) {
		bam_data_free_skb_to_pool(port, skb);

		pr_err("%s: data rx enqueue err %d\n", __func__, status);

		spin_lock(&port->port_lock_ul);
		list_add_tail(&req->list, &d->rx_idle);
		spin_unlock(&port->port_lock_ul);
	}
}

static int _bam_data_start_io(struct bam_data_port *port, bool in)
{
	int			ret;
	struct usb_ep		*ep;
	struct list_head	*idle;
	unsigned		queue_size;
	void		(*ep_complete)(struct usb_ep *, struct usb_request *);


	if (!port->port_usb)
		return -EBUSY;
	if (in)
		return -ENODEV;

	ep = port->port_usb->out;
	idle = &port->data_ch.rx_idle;
	queue_size = bam_data_rx_q_size;
	ep_complete = bam_data_epout_complete;

	ret = bam_data_alloc_requests(ep, idle, queue_size, ep_complete,
			GFP_ATOMIC);
	if (ret) {
		pr_err("%s: allocation failed\n", __func__);
		return ret;
	}

	return 0;
}

static void bam_data_write_toipa(struct work_struct *w)
{
	struct bam_data_port	*port;
	struct bam_data_ch_info	*d;
	struct sk_buff		*skb;
	int			ret;
	int			qlen;
	unsigned long		flags;

	d = container_of(w, struct bam_data_ch_info, write_tobam_w);
	port = d->port;

	spin_lock_irqsave(&port->port_lock_ul, flags);
	if (!port->port_usb) {
		spin_unlock_irqrestore(&port->port_lock_ul, flags);
		return;
	}

	while (d->pending_with_bam < BAM_PENDING_LIMIT &&
	       usb_bam_get_prod_granted(d->dst_connection_idx)) {
		skb =  __skb_dequeue(&d->rx_skb_q);
		if (!skb)
			break;

		d->pending_with_bam++;

		pr_debug("%s: port:%p d:%p pbam:%u pno:%d\n", __func__,
				port, d, d->pending_with_bam, port->port_num);

		spin_unlock_irqrestore(&port->port_lock_ul, flags);
		ret = ipa_tx_dp(IPA_CLIENT_USB_PROD, skb, NULL);
		spin_lock_irqsave(&port->port_lock_ul, flags);
		if (ret) {
			pr_debug("%s: write error:%d\n", __func__, ret);
			d->pending_with_bam--;
			spin_unlock_irqrestore(&port->port_lock_ul, flags);
			bam_data_free_skb_to_pool(port, skb);
			spin_lock_irqsave(&port->port_lock_ul, flags);
			break;
		}
	}

	qlen = d->rx_skb_q.qlen;

	spin_unlock_irqrestore(&port->port_lock_ul, flags);

	if (qlen < bam_ipa_rx_fctrl_dis_thld) {
		if (d->rx_flow_control_triggered) {
			d->rx_flow_control_disable++;
			d->rx_flow_control_triggered = 0;
		}
		bam_data_start_rx(port);
	}
}

/*------------data_path----------------------------*/

static void bam_data_endless_rx_complete(struct usb_ep *ep,
					 struct usb_request *req)
{
	int status = req->status;

	pr_debug("%s: status: %d\n", __func__, status);
}

static void bam_data_endless_tx_complete(struct usb_ep *ep,
					 struct usb_request *req)
{
	int status = req->status;

	pr_debug("%s: status: %d\n", __func__, status);
}

static void bam_data_start_endless_rx(struct bam_data_port *port)
{
	struct bam_data_ch_info *d = &port->data_ch;
	int status;

	spin_lock(&port->port_lock_ul);
	if (!port->port_usb) {
		spin_unlock(&port->port_lock_ul);
		return;
	}

	pr_debug("%s: enqueue\n", __func__);
	status = usb_ep_queue(port->port_usb->out, d->rx_req, GFP_ATOMIC);
	if (status)
		pr_err("error enqueuing transfer, %d\n", status);

	spin_unlock(&port->port_lock_ul);
}

static void bam_data_start_endless_tx(struct bam_data_port *port)
{
	struct bam_data_ch_info *d = &port->data_ch;
	int status;

	if (!port->port_usb)
		return;

	pr_debug("%s: enqueue\n", __func__);
	status = usb_ep_queue(port->port_usb->in, d->tx_req, GFP_ATOMIC);
	if (status)
		pr_err("error enqueuing transfer, %d\n", status);
}

static void bam_data_stop_endless_rx(struct bam_data_port *port)
{
	struct bam_data_ch_info *d = &port->data_ch;
	int status;

	spin_lock(&port->port_lock_ul);
	if (!port->port_usb) {
		spin_unlock(&port->port_lock_ul);
		return;
	}

	pr_debug("%s: dequeue\n", __func__);
	status = usb_ep_dequeue(port->port_usb->out, d->rx_req);
	if (status)
		pr_err("%s: error dequeuing transfer, %d\n", __func__, status);

	spin_unlock(&port->port_lock_ul);
}
static void bam_data_stop_endless_tx(struct bam_data_port *port)
{
	struct bam_data_ch_info *d = &port->data_ch;
	int status;

	if (!port->port_usb)
		return;

	pr_debug("%s: dequeue\n", __func__);
	status = usb_ep_dequeue(port->port_usb->in, d->tx_req);
	if (status)
		pr_err("%s: error dequeuing transfer, %d\n", __func__, status);
}

static int bam_data_peer_reset_cb(void *param)
{
	struct bam_data_port	*port = (struct bam_data_port *)param;
	struct bam_data_ch_info *d;
	int ret;

	d = &port->data_ch;

	pr_debug("%s: reset by peer\n", __func__);

	/* Reset BAM */
	ret = usb_bam_a2_reset(0);
	if (ret) {
		pr_err("%s: BAM reset failed %d\n", __func__, ret);
		return ret;
	}

	/* Unregister the peer reset callback */
	usb_bam_register_peer_reset_cb(NULL, NULL);

	return 0;
}

static void bam2bam_data_disconnect_work(struct work_struct *w)
{
	struct bam_data_port *port =
			container_of(w, struct bam_data_port, disconnect_w);
	struct bam_data_ch_info *d = &port->data_ch;
	int ret;
	void *priv;

	if (!port->is_connected) {
		pr_info("%s: Already disconnected. Bailing out.\n", __func__);
		return;
	}

	if (d->trans == USB_GADGET_XPORT_BAM2BAM_IPA) {
		if (d->src_pipe_type == USB_BAM_PIPE_BAM2BAM)
			priv = d->ipa_params.priv;
		else
			priv = d->ul_params.teth_priv;

		if (d->func_type == USB_FUNC_ECM) {
			ecm_ipa_disconnect(priv);
		} else if (d->func_type == USB_FUNC_RNDIS) {
			rndis_ipa_pipe_disconnect_notify(priv);
			is_ipa_rndis_net_on = false;
		}

		ret = usb_bam_disconnect_ipa(&d->ipa_params);
		if (!ret) {
			pr_debug("%s(): freeing bam2bam_data_port\n", __func__);
			bam2bam_data_port_free(0);
		} else {
			pr_err("usb_bam_disconnect_ipa failed: err:%d\n", ret);
		}

		if (d->func_type == USB_FUNC_MBIM)
			teth_bridge_disconnect(d->ipa_params.src_client);

	}

	port->is_connected = false;
	pr_debug("Disconnect workqueue done (port %p)\n", port);
}
/*
 * This function configured data fifo based on index passed to get bam2bam
 * configuration.
 */
static void configure_usb_data_fifo(u8 idx, struct usb_ep *ep,
		enum usb_bam_pipe_type pipe_type)
{
	struct u_bam_data_connect_info bam_info;
	struct sps_mem_buffer data_fifo = {0};

	if (pipe_type == USB_BAM_PIPE_BAM2BAM) {
		get_bam2bam_connection_info(idx,
					&bam_info.usb_bam_handle,
					&bam_info.usb_bam_pipe_idx,
					&bam_info.peer_pipe_idx,
					NULL, &data_fifo, NULL);

		msm_data_fifo_config(ep,
					data_fifo.phys_base,
					data_fifo.size,
					bam_info.usb_bam_pipe_idx);
	}
}

static void bam2bam_data_connect_work(struct work_struct *w)
{
	struct bam_data_port *port = container_of(w, struct bam_data_port,
						  connect_w);
	struct teth_bridge_connect_params connect_params;
	struct teth_bridge_init_params teth_bridge_params;
	struct bam_data_ch_info *d = &port->data_ch;
	struct data_port	*d_port = port->port_usb;
	struct usb_gadget	*gadget = NULL;
	u32			sps_params;
	int			ret;
	unsigned long		flags;

	pr_debug("%s: Connect workqueue started", __func__);

	if (port->is_connected) {
		pr_info("%s: Already connected. Bailing out.\n", __func__);
		return;
	}

	if (d_port && d_port->cdev)
		gadget = d_port->cdev->gadget;

	if (!gadget) {
		pr_err("%s: NULL gadget\n", __func__);
		return;
	}

	spin_lock_irqsave(&port->port_lock_ul, flags);
	if (!port->port_usb) {
		spin_unlock_irqrestore(&port->port_lock_ul, flags);
		pr_err("port_usb is NULL");
		return;
	}

	if (!port->port_usb->out) {
		spin_unlock_irqrestore(&port->port_lock_ul, flags);
		pr_err("port_usb->out (bulk out ep) is NULL");
		return;
	}

	d->rx_req = usb_ep_alloc_request(port->port_usb->out, GFP_ATOMIC);
	if (!d->rx_req) {
		spin_unlock_irqrestore(&port->port_lock_ul, flags);
		pr_err("%s: failed to allocate rx_req\n", __func__);
		return;
	}

	d->rx_req->context = port;
	d->rx_req->complete = bam_data_endless_rx_complete;
	d->rx_req->length = 0;
	d->rx_req->no_interrupt = 1;

	d->tx_req = usb_ep_alloc_request(port->port_usb->in, GFP_ATOMIC);
	if (!d->tx_req) {
		spin_unlock_irqrestore(&port->port_lock_ul, flags);
		pr_err("%s: failed to allocate tx_req\n", __func__);
		return;
	}
	spin_unlock_irqrestore(&port->port_lock_ul, flags);

	d->tx_req->context = port;
	d->tx_req->complete = bam_data_endless_tx_complete;
	d->tx_req->length = 0;
	d->tx_req->no_interrupt = 1;

	if (d->trans == USB_GADGET_XPORT_BAM2BAM_IPA) {

		d->ipa_params.usb_connection_speed = gadget->speed;

		if (d->dst_pipe_type != USB_BAM_PIPE_BAM2BAM) {
			pr_err("%s: no software preparation for DL not using bam2bam\n",
					__func__);
			return;
		}

		if (d->func_type == USB_FUNC_MBIM) {
			teth_bridge_params.client = d->ipa_params.src_client;
			ret = teth_bridge_init(&teth_bridge_params);
			if (ret) {
				pr_err("%s:teth_bridge_init() failed\n",
				      __func__);
				return;
			}
			d->ipa_params.notify =
				teth_bridge_params.usb_notify_cb;
			d->ipa_params.priv =
				teth_bridge_params.private_data;
			d->ipa_params.ipa_ep_cfg.mode.mode = IPA_BASIC;
			d->ipa_params.skip_ep_cfg =
				teth_bridge_params.skip_ep_cfg;
		}
		d->ipa_params.dir = USB_TO_PEER_PERIPHERAL;
		if (d->func_type == USB_FUNC_ECM) {
			d->ipa_params.notify = ecm_qc_get_ipa_rx_cb();
			d->ipa_params.priv = ecm_qc_get_ipa_priv();
			d->ipa_params.skip_ep_cfg = ecm_qc_get_skip_ep_config();
		}

		if (d->func_type == USB_FUNC_RNDIS) {
			d->ipa_params.notify = rndis_qc_get_ipa_rx_cb();
			d->ipa_params.priv = rndis_qc_get_ipa_priv();
			d->ipa_params.skip_ep_cfg =
				rndis_qc_get_skip_ep_config();
		}

		/* Support for UL using system-to-IPA */
		if (d->src_pipe_type == USB_BAM_PIPE_SYS2BAM) {
			d->ul_params.teth_cb = d->ipa_params.notify;
			d->ipa_params.notify =
				bam_data_ipa_sys2bam_notify_cb;
			d->ul_params.teth_priv = d->ipa_params.priv;
			d->ipa_params.priv = &d->ul_params;
			d->ipa_params.reset_pipe_after_lpm = false;
		} else {
			d->ipa_params.reset_pipe_after_lpm =
				(gadget_is_dwc3(gadget) &&
				 msm_dwc3_reset_ep_after_lpm(gadget));
		}

		ret = usb_bam_connect_ipa(&d->ipa_params);
		if (ret) {
			pr_err("%s: usb_bam_connect_ipa failed: err:%d\n",
				__func__, ret);
			return;
		}

		d_port->ipa_consumer_ep = d->ipa_params.ipa_cons_ep_idx;

		d->src_bam_idx = usb_bam_get_connection_idx(
				gadget->name,
				IPA_P_BAM, USB_TO_PEER_PERIPHERAL,
				USB_BAM_DEVICE, 0);
		if (d->src_bam_idx < 0) {
			pr_err("%s: get_connection_idx failed\n",
				__func__);
			return;
		}

		if (gadget_is_dwc3(gadget))
			configure_usb_data_fifo(d->src_bam_idx,
					port->port_usb->out,
					d->src_pipe_type);

		/* Remove support for UL using system-to-IPA towards DL */
		if (d->src_pipe_type == USB_BAM_PIPE_SYS2BAM) {
			d->ipa_params.notify = d->ul_params.teth_cb;
			d->ipa_params.priv = d->ul_params.teth_priv;
		}

		d->ipa_params.dir = PEER_PERIPHERAL_TO_USB;
		if (d->func_type == USB_FUNC_ECM) {
			d->ipa_params.notify = ecm_qc_get_ipa_tx_cb();
			d->ipa_params.priv = ecm_qc_get_ipa_priv();
			d->ipa_params.skip_ep_cfg = ecm_qc_get_skip_ep_config();
		}
		if (d->func_type == USB_FUNC_RNDIS) {
			d->ipa_params.notify = rndis_qc_get_ipa_tx_cb();
			d->ipa_params.priv = rndis_qc_get_ipa_priv();
			d->ipa_params.skip_ep_cfg =
				rndis_qc_get_skip_ep_config();
		}

		if (d->dst_pipe_type == USB_BAM_PIPE_BAM2BAM) {
			d->ipa_params.reset_pipe_after_lpm =
				(gadget_is_dwc3(gadget) &&
				 msm_dwc3_reset_ep_after_lpm(gadget));
		} else {
			d->ipa_params.reset_pipe_after_lpm = false;
		}

		ret = usb_bam_connect_ipa(&d->ipa_params);
		if (ret) {
			pr_err("%s: usb_bam_connect_ipa failed: err:%d\n",
				__func__, ret);
			return;
		}

		d_port->ipa_producer_ep = d->ipa_params.ipa_prod_ep_idx;
		pr_debug("%s(): ipa_producer_ep:%d ipa_consumer_ep:%d\n",
				__func__, d_port->ipa_producer_ep,
				d_port->ipa_consumer_ep);

		d->dst_bam_idx = usb_bam_get_connection_idx(
				gadget->name,
				IPA_P_BAM, PEER_PERIPHERAL_TO_USB,
				USB_BAM_DEVICE, 0);
		if (d->dst_bam_idx < 0) {
			pr_err("%s: get_connection_idx failed\n",
				__func__);
			return;
		}

		if (gadget_is_dwc3(gadget))
			configure_usb_data_fifo(d->dst_bam_idx,
					port->port_usb->in,
					d->dst_pipe_type);

		if (d->func_type == USB_FUNC_MBIM) {
			connect_params.ipa_usb_pipe_hdl =
				d->ipa_params.prod_clnt_hdl;
			connect_params.usb_ipa_pipe_hdl =
				d->ipa_params.cons_clnt_hdl;
			connect_params.tethering_mode =
				TETH_TETHERING_MODE_MBIM;
			connect_params.client_type = d->ipa_params.src_client;
			ret = teth_bridge_connect(&connect_params);
			if (ret) {
				pr_err("%s:teth_bridge_connect() failed\n",
				      __func__);
				return;
			}
		}

		if (d->func_type == USB_FUNC_ECM) {
			ret = ecm_ipa_connect(d->ipa_params.cons_clnt_hdl,
				d->ipa_params.prod_clnt_hdl,
				d->ipa_params.priv);
			if (ret) {
				pr_err("%s: failed to connect IPA: err:%d\n",
					__func__, ret);
				return;
			}
		}
		if (d->func_type == USB_FUNC_RNDIS) {
			rndis_data.prod_clnt_hdl =
				d->ipa_params.prod_clnt_hdl;
			rndis_data.cons_clnt_hdl =
				d->ipa_params.cons_clnt_hdl;
			rndis_data.priv = d->ipa_params.priv;

			ret = rndis_ipa_pipe_connect_notify(
				rndis_data.cons_clnt_hdl,
				rndis_data.prod_clnt_hdl,
				rndis_data.max_transfer_size,
				rndis_data.max_packets_number,
				rndis_data.priv);
			if (ret) {
				pr_err("%s: failed to connect IPA: err:%d\n",
					__func__, ret);
				return;
			}
			is_ipa_rndis_net_on = true;
		}
	} else { /* transport type is USB_GADGET_XPORT_BAM2BAM */
		usb_bam_reset_complete();
		/* Setup BAM connection and fetch USB PIPE index */
		ret = usb_bam_connect(d->src_connection_idx, &d->src_pipe_idx);
		if (ret) {
			pr_err("usb_bam_connect (src) failed: err:%d\n", ret);
			return;
		}
		ret = usb_bam_connect(d->dst_connection_idx, &d->dst_pipe_idx);
		if (ret) {
			pr_err("usb_bam_connect (dst) failed: err:%d\n", ret);
			return;
		}
	}
	/* Upadate BAM specific attributes in usb_request */
	if (gadget_is_dwc3(gadget)) {
		sps_params = MSM_SPS_MODE | MSM_DISABLE_WB | MSM_PRODUCER |
				d->src_pipe_idx;
		d->rx_req->length = 32*1024;
	} else {
		sps_params = (SPS_PARAMS_SPS_MODE | d->src_pipe_idx |
			MSM_VENDOR_ID) & ~SPS_PARAMS_TBE;
	}
	d->rx_req->udc_priv = sps_params;

	if (gadget_is_dwc3(gadget)) {
		sps_params = MSM_SPS_MODE | MSM_DISABLE_WB | d->dst_pipe_idx;
		d->tx_req->length = 32*1024;
	} else {
		sps_params = (SPS_PARAMS_SPS_MODE | d->dst_pipe_idx |
			MSM_VENDOR_ID) & ~SPS_PARAMS_TBE;
	}
	d->tx_req->udc_priv = sps_params;

	/* queue in & out requests */
	if (d->trans == USB_GADGET_XPORT_BAM2BAM ||
		d->src_pipe_type == USB_BAM_PIPE_BAM2BAM)
		bam_data_start_endless_rx(port);
	else {
		/* The use-case of UL (OUT) ports using sys2bam is based on
		 * partial reuse of the system-to-bam_demux code. The following
		 * lines perform the branching out of the standard bam2bam flow
		 * on the USB side of the UL channel
		 */
		if (_bam_data_start_io(port, false)) {
			pr_err("%s: _bam_data_start_io\n", __func__);
			return;
		}
		bam_data_start_rx(port);
	}
	bam_data_start_endless_tx(port);

	/* Register for peer reset callback if USB_GADGET_XPORT_BAM2BAM */
	if (d->trans != USB_GADGET_XPORT_BAM2BAM_IPA) {
		usb_bam_register_peer_reset_cb(bam_data_peer_reset_cb, port);

		ret = usb_bam_client_ready(true);
		if (ret) {
			pr_err("%s: usb_bam_client_ready failed: err:%d\n",
			__func__, ret);
			return;
		}
	}

	port->is_connected = true;
	pr_debug("Connect workqueue done (port %p)", port);
}

static void bam2bam_data_port_free(int portno)
{
	struct bam_data_port *port = bam2bam_data_ports[portno];

	if (port == NULL) {
		pr_debug("port %d already free\n", portno);
		return;
	}

	if (--port->ref_count == 0) {
		kfree(port);
		port = NULL;
		n_bam2bam_data_ports--;
		pr_debug("freed port %d\n", portno);
	}
}

static int bam2bam_data_port_alloc(int portno)
{
	struct bam_data_port	*port = NULL;
	struct bam_data_ch_info	*d = NULL;

	if (bam2bam_data_ports[portno] != NULL) {
		pr_debug("port %d already allocated. incremeting ref_count\n",
				portno);
		bam2bam_data_ports[portno]->ref_count++;
		goto done;
	}

	port = kzalloc(sizeof(struct bam_data_port), GFP_KERNEL);
	if (!port) {
		pr_err("no memory to allocate port %d\n", portno);
		return -ENOMEM;
	}

	port->port_num  = portno;
	port->ref_count = 1;
	port->is_connected = false;

	spin_lock_init(&port->port_lock_ul);
	INIT_WORK(&port->connect_w, bam2bam_data_connect_work);
	INIT_WORK(&port->disconnect_w, bam2bam_data_disconnect_work);
	INIT_WORK(&port->suspend_w, bam2bam_data_suspend_work);
	INIT_WORK(&port->resume_w, bam2bam_data_resume_work);

	/* data ch */
	d = &port->data_ch;
	d->port = port;
	bam2bam_data_ports[portno] = port;

	/* UL workaround requirements */
	skb_queue_head_init(&d->rx_skb_q);
	skb_queue_head_init(&d->rx_skb_idle);
	INIT_LIST_HEAD(&d->rx_idle);
	INIT_WORK(&d->write_tobam_w, bam_data_write_toipa);

	rndis_disconn_w = &port->disconnect_w;

done:
	pr_debug("port:%p portno:%d\n", port, portno);

	return 0;
}

void u_bam_data_start_rndis_ipa(void)
{
	pr_debug("%s\n", __func__);

	if (!is_ipa_rndis_net_on)
		queue_work(bam_data_wq, rndis_conn_w);
}

void u_bam_data_stop_rndis_ipa(void)
{
	pr_debug("%s\n", __func__);

	if (is_ipa_rndis_net_on)
		queue_work(bam_data_wq, rndis_disconn_w);
}

void bam_data_disconnect(struct data_port *gr, u8 port_num)
{
	struct bam_data_port	*port;
	struct bam_data_ch_info	*d;

	pr_debug("dev:%p port#%d\n", gr, port_num);

	if (port_num >= n_bam2bam_data_ports) {
		pr_err("invalid bam2bam portno#%d\n", port_num);
		return;
	}

	if (!gr) {
		pr_err("data port is null\n");
		return;
	}

	port = bam2bam_data_ports[port_num];

	if (!port) {
		pr_err("port %u is NULL", port_num);
		return;
	}

	d = &port->data_ch;
	if (port->port_usb) {
		if (d->trans == USB_GADGET_XPORT_BAM2BAM_IPA) {
			port->port_usb->ipa_consumer_ep = -1;
			port->port_usb->ipa_producer_ep = -1;
		}
		if (port->port_usb->in && port->port_usb->in->driver_data) {
			/* disable endpoints */
			usb_ep_disable(port->port_usb->out);
			usb_ep_disable(port->port_usb->in);

			/*
			 * Set endless flag to false as USB Endpoint
			 * is already disable.
			 */
			if (d->trans == USB_GADGET_XPORT_BAM2BAM ||
				d->trans == USB_GADGET_XPORT_BAM2BAM_IPA ||
				d->trans == USB_GADGET_XPORT_BAM) {

				if (d->dst_pipe_type == USB_BAM_PIPE_BAM2BAM)
					port->port_usb->in->endless = false;

				if (d->src_pipe_type == USB_BAM_PIPE_BAM2BAM)
					port->port_usb->out->endless = false;
			}

			port->port_usb->in->driver_data = NULL;
			port->port_usb->out->driver_data = NULL;

			port->port_usb = NULL;
		}
	}

	if (d->trans == USB_GADGET_XPORT_BAM2BAM_IPA) {
		queue_work(bam_data_wq, &port->disconnect_w);
	} else {
		if (usb_bam_client_ready(false))
			pr_err("%s: usb_bam_client_ready failed\n",
				__func__);
	}
}

int bam_data_connect(struct data_port *gr, u8 port_num,
	enum transport_type trans, u8 src_connection_idx,
	u8 dst_connection_idx, enum function_type func)
{
	struct bam_data_port	*port;
	struct bam_data_ch_info	*d;
	int			ret;
	unsigned long		flags;

	pr_debug("dev:%p port#%d\n", gr, port_num);

	if (port_num >= n_bam2bam_data_ports) {
		pr_err("invalid portno#%d\n", port_num);
		return -ENODEV;
	}

	if (!gr) {
		pr_err("data port is null\n");
		return -ENODEV;
	}

	port = bam2bam_data_ports[port_num];

	spin_lock_irqsave(&port->port_lock_ul, flags);
	port->port_usb = gr;
	d = &port->data_ch;
	spin_unlock_irqrestore(&port->port_lock_ul, flags);

	d->src_connection_idx = src_connection_idx;
	d->dst_connection_idx = dst_connection_idx;

	d->trans = trans;
	d->func_type = func;
	d->rx_buffer_size = (gr->rx_buffer_size ? gr->rx_buffer_size :
					bam_mux_rx_req_size);

	/*
	 * Both source (consumer) and destination (producer) use the same
	 * controller, so checking just one of them should suffice.
	 */
	if (usb_bam_get_bam_type(src_connection_idx) == HSIC_CTRL) {
		d->ipa_params.src_client = IPA_CLIENT_HSIC1_PROD;
		d->ipa_params.dst_client = IPA_CLIENT_HSIC1_CONS;
	} else {
		d->ipa_params.src_client = IPA_CLIENT_USB_PROD;
		d->ipa_params.dst_client = IPA_CLIENT_USB_CONS;
	}

	pr_debug("%s(): rx_buffer_size:%d\n", __func__, d->rx_buffer_size);
	if (trans == USB_GADGET_XPORT_BAM2BAM_IPA) {
		d->ipa_params.src_pipe = &(d->src_pipe_idx);
		d->ipa_params.dst_pipe = &(d->dst_pipe_idx);
		d->ipa_params.src_idx = src_connection_idx;
		d->ipa_params.dst_idx = dst_connection_idx;
		d->rx_flow_control_disable = 0;
		d->rx_flow_control_enable = 0;
		d->rx_flow_control_triggered = 0;

		/*
		 * Query pipe type using IPA src/dst index with
		 * usbbam driver. It is being set either as
		 * BAM2BAM or SYS2BAM.
		 */
		if (usb_bam_get_pipe_type(d->ipa_params.src_idx,
			&d->src_pipe_type) ||
			usb_bam_get_pipe_type(d->ipa_params.dst_idx,
			&d->dst_pipe_type)) {
			pr_err("usb_bam_get_pipe_type() failed\n");
			return -EINVAL;
		}
	}

	/*
	 * Check for pipe_type. If it is BAM2BAM, then it is required
	 * to disable Xfer complete and Xfer not ready interrupts for
	 * that particular endpoint. Hence it set endless flag based
	 * it which is considered into UDC driver while enabling
	 * USB Endpoint.
	 */
	if (d->trans == USB_GADGET_XPORT_BAM2BAM ||
		d->trans == USB_GADGET_XPORT_BAM2BAM_IPA ||
		d->trans == USB_GADGET_XPORT_BAM) {

		if (d->dst_pipe_type == USB_BAM_PIPE_BAM2BAM)
			port->port_usb->in->endless = true;

		if (d->src_pipe_type == USB_BAM_PIPE_BAM2BAM)
			port->port_usb->out->endless = true;
	}

	ret = usb_ep_enable(gr->in);
	if (ret) {
		pr_err("usb_ep_enable failed eptype:IN ep:%p", gr->in);
		goto exit;
	}

	gr->in->driver_data = port;

	ret = usb_ep_enable(gr->out);
	if (ret) {
		pr_err("usb_ep_enable failed eptype:OUT ep:%p", gr->out);
		gr->in->driver_data = 0;
		goto exit;
	}

	gr->out->driver_data = port;


	if (d->trans == USB_GADGET_XPORT_BAM2BAM_IPA && d->func_type ==
		USB_FUNC_RNDIS) {
			rndis_conn_w = &port->connect_w;
			ret = 0;
			goto exit;
	}

	queue_work(bam_data_wq, &port->connect_w);
	ret = 0;

exit:
	return ret;
}

int bam_data_destroy(unsigned int no_bam2bam_port)
{
	struct bam_data_ch_info	*d;
	struct bam_data_port	*port;

	port = bam2bam_data_ports[no_bam2bam_port];
	d = &port->data_ch;

	pr_debug("bam_data_destroy: Freeing ports\n");
	bam2bam_data_port_free(no_bam2bam_port);
	if (bam_data_wq)
		destroy_workqueue(bam_data_wq);
	bam_data_wq = NULL;

	return 0;
}

void bam_work_destroy(void)
{
	if (bam_data_wq)
		destroy_workqueue(bam_data_wq);
	bam_data_wq = NULL;
}

int bam_data_setup(unsigned int no_bam2bam_port)
{
	int	i;
	int	ret;

	pr_debug("requested %d BAM2BAM ports", no_bam2bam_port);

	if (!no_bam2bam_port || no_bam2bam_port > BAM2BAM_DATA_N_PORTS) {
		pr_err("Invalid num of ports count:%d\n", no_bam2bam_port);
		return -EINVAL;
	}

	if (bam_data_wq) {
		pr_debug("bam_data is already setup");
		return 0;
	}

	bam_data_wq = alloc_workqueue("k_bam_data",
				      WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
	if (!bam_data_wq) {
		pr_err("Failed to create workqueue\n");
		return -ENOMEM;
	}

	for (i = 0; i < no_bam2bam_port; i++) {
		n_bam2bam_data_ports++;
		ret = bam2bam_data_port_alloc(i);
		if (ret) {
			n_bam2bam_data_ports--;
			pr_err("Failed to alloc port:%d\n", i);
			goto free_bam_ports;
		}
	}

	return 0;

free_bam_ports:
	for (i = 0; i < n_bam2bam_data_ports; i++)
		bam2bam_data_port_free(i);
	destroy_workqueue(bam_data_wq);

	return ret;
}

static int bam_data_wake_cb(void *param)
{
	struct bam_data_port *port = (struct bam_data_port *)param;
	struct data_port *d_port = port->port_usb;

	pr_debug("%s: woken up by peer\n", __func__);

	if (!d_port) {
		pr_err("FAILED: d_port == NULL");
		return -ENODEV;
	}

	if (!d_port->cdev) {
		pr_err("FAILED: d_port->cdev == NULL");
		return -ENODEV;
	}

	if (!d_port->cdev->gadget) {
		pr_err("FAILED: d_port->cdev->gadget == NULL");
		return -ENODEV;
	}

	return usb_gadget_wakeup(d_port->cdev->gadget);
}

static void bam_data_start(void *param, enum usb_bam_pipe_dir dir)
{
	struct bam_data_port *port = param;
	struct data_port *d_port = port->port_usb;
	struct bam_data_ch_info *d = &port->data_ch;
	struct usb_gadget *gadget;

	if (!d_port || !d_port->cdev || !d_port->cdev->gadget) {
		pr_err("%s:d_port,cdev or gadget is  NULL\n", __func__);
		return;
	}

	gadget = d_port->cdev->gadget;

	if (dir == USB_TO_PEER_PERIPHERAL) {
		if (port->data_ch.src_pipe_type == USB_BAM_PIPE_BAM2BAM)
			bam_data_start_endless_rx(port);
		else {
			bam_data_start_rx(port);
			queue_work(bam_data_wq, &d->write_tobam_w);
		}
	} else {
		if (gadget_is_dwc3(gadget) &&
		    msm_dwc3_reset_ep_after_lpm(gadget)) {
			u8 idx;

			idx = usb_bam_get_connection_idx(gadget->name,
				IPA_P_BAM, PEER_PERIPHERAL_TO_USB,
				USB_BAM_DEVICE, 0);
			if (idx < 0) {
				pr_err("%s: get_connection_idx failed\n",
					__func__);
				return;
			}
			configure_data_fifo(idx,
				port->port_usb->in,
				d->dst_pipe_type);
		}
		bam_data_start_endless_tx(port);
	}

}

static void bam_data_stop(void *param, enum usb_bam_pipe_dir dir)
{
	struct bam_data_port *port = param;

	if (dir == USB_TO_PEER_PERIPHERAL) {
		if (port->data_ch.src_pipe_type == USB_BAM_PIPE_BAM2BAM)
			bam_data_stop_endless_rx(port);
		else
			pr_warn("%s: no function equivalent to bam_data_stop_endless_rx for sys2bam pipe\n",
					__func__);
	} else {
		bam_data_stop_endless_tx(port);
	}
}

void bam_data_suspend(u8 port_num)
{
	struct bam_data_port	*port;
	struct bam_data_ch_info *d;

	port = bam2bam_data_ports[port_num];
	d = &port->data_ch;

	pr_debug("%s: suspended port %d\n", __func__, port_num);

	queue_work(bam_data_wq, &port->suspend_w);
}

void bam_data_resume(u8 port_num)
{

	struct bam_data_port	*port;
	struct bam_data_ch_info *d;

	port = bam2bam_data_ports[port_num];
	d = &port->data_ch;

	pr_debug("%s: resumed port %d\n", __func__, port_num);

	queue_work(bam_data_wq, &port->resume_w);
}

static void bam2bam_data_suspend_work(struct work_struct *w)
{
	struct bam_data_port *port =
			container_of(w, struct bam_data_port, suspend_w);
	struct bam_data_ch_info *d = &port->data_ch;
	int ret;

	pr_debug("%s: suspend work started\n", __func__);

	if (!port->is_connected) {
		pr_info("%s: Port is disconnected. Bailing out.\n", __func__);
		return;
	}

	ret = usb_bam_register_wake_cb(d->dst_connection_idx,
					bam_data_wake_cb, port);
	if (ret) {
		pr_err("%s(): Failed to register BAM wake callback.\n",
			__func__);
		return;
	}

	if (d->trans == USB_GADGET_XPORT_BAM2BAM_IPA) {
		usb_bam_register_start_stop_cbs(d->dst_connection_idx,
						bam_data_start, bam_data_stop,
						port);
		usb_bam_suspend(&d->ipa_params);
	}
}

static void bam2bam_data_resume_work(struct work_struct *w)
{
	struct bam_data_port *port =
			container_of(w, struct bam_data_port, resume_w);
	struct bam_data_ch_info *d = &port->data_ch;
	struct data_port *d_port = port->port_usb;
	struct usb_gadget *gadget = d_port->cdev->gadget;
	int ret;

	pr_debug("%s: resume work started\n", __func__);

	if (!port->is_connected) {
		pr_info("%s: Port is disconnected. Bailing out.\n", __func__);
		return;
	}

	ret = usb_bam_register_wake_cb(d->dst_connection_idx, NULL, NULL);
	if (ret) {
		pr_err("%s(): Failed to un-register BAM wake callback.\n",
			__func__);
		return;
	}

	if (d->trans == USB_GADGET_XPORT_BAM2BAM_IPA) {
		if (gadget_is_dwc3(gadget) &&
			msm_dwc3_reset_ep_after_lpm(gadget)) {
				configure_usb_data_fifo(d->src_bam_idx,
					port->port_usb->out,
					d->src_pipe_type);
				configure_usb_data_fifo(d->dst_bam_idx,
					port->port_usb->in,
					d->dst_pipe_type);
				msm_dwc3_reset_dbm_ep(port->port_usb->in);
		}
		usb_bam_resume(&d->ipa_params);
	}
}

void u_bam_data_set_max_xfer_size(u32 max_transfer_size)
{
	if (!max_transfer_size) {
		pr_err("%s: invalid parameters\n", __func__);
		return;
	}

	rndis_data.max_transfer_size = max_transfer_size;
}

void u_bam_data_set_max_pkt_num(u32 max_packets_number)

{
	if (!max_packets_number) {
		pr_err("%s: invalid parameters\n", __func__);
		return;
	}

	rndis_data.max_packets_number = max_packets_number;
}

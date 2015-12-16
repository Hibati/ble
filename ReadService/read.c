#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/uuid.h>
#include <gatt/gatt.h>
#include <gatt/gattrib.h>
#include <gatt/btio.h>

#define DEFAULT_DEFER_TIMEOUT 30
#define ERROR_FAILED(gerr, str, err) \
		g_set_error(gerr, BT_IO_ERROR, err, \
				str ": %s (%d)", strerror(err), err)
static GAttrib *attrib = NULL;
static char *opt_dst_type = NULL;
//static char *opt_value = NULL;
static char *opt_sec_level = NULL;
//static bt_uuid_t *opt_uuid = NULL;
//static int opt_start = 0x0001;
//static int opt_end = 0xffff;
//static int opt_handle = -1;
static int opt_mtu = 23;
static int opt_psm = 0;
typedef enum {
	BT_IO_L2CAP,
	BT_IO_RFCOMM,
	BT_IO_SCO,
	BT_IO_INVALID,
} BtIOType;

struct connect {
	BtIOConnect connect;
	gpointer user_data;
	GDestroyNotify destroy;
};
struct set_opts {
	bdaddr_t src;
	bdaddr_t dst;
	BtIOType type;
	uint8_t src_type;
	uint8_t dst_type;
	int defer;
	int sec_level;
	uint8_t channel;
	uint16_t psm;
	uint16_t cid;
	uint16_t mtu;
	uint16_t imtu;
	uint16_t omtu;
	int master;
	uint8_t mode;
	int flushable;
	uint32_t priority;
	uint16_t voice;
};
void rl_printf(const char *fmt, ...)
{
	va_list args;
	bool save_input;
	char *saved_line;
	int saved_point;

	save_input = !RL_ISSTATE(RL_STATE_DONE);

	if (save_input) {
		saved_point = rl_point;
		saved_line = rl_copy_text(0, rl_end);
		rl_save_prompt();
		rl_replace_line("", 0);
		rl_redisplay();
	}

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	if (save_input) {
		rl_restore_prompt();
		rl_replace_line(saved_line, 0);
		rl_point = saved_point;
		rl_redisplay();
		free(saved_line);
	}
}
static void primary_all_cb(GSList *services, guint8 status, gpointer user_data)
{
	GSList *l;

	if (status) {
		error("Discover all primary services failed: %s\n",
						att_ecode2str(status));
		return;
	}

	if (services == NULL) {
		error("No primary service found\n");
		return;
	}

	for (l = services; l; l = l->next) {
		struct gatt_primary *prim = l->data;
		rl_printf("attr handle: 0x%04x, end grp handle: 0x%04x uuid: %s\n",
				prim->range.start, prim->range.end, prim->uuid);
	}
}
static void events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data)
{
	uint8_t *opdu;
	uint16_t handle, i, olen;
	size_t plen;
	GString *s;

	handle = att_get_u16(&pdu[1]);

	switch (pdu[0]) {
	case ATT_OP_HANDLE_NOTIFY:
		s = g_string_new(NULL);
		g_string_printf(s, "Notification handle = 0x%04x value: ",
									handle);
		break;
	case ATT_OP_HANDLE_IND:
		s = g_string_new(NULL);
		g_string_printf(s, "Indication   handle = 0x%04x value: ",
									handle);
		break;
	default:
		error("Invalid opcode\n");
		return;
	}

	for (i = 3; i < len; i++)
		g_string_append_printf(s, "%02x ", pdu[i]);

	rl_printf("%s\n", s->str);
	g_string_free(s, TRUE);

	if (pdu[0] == ATT_OP_HANDLE_NOTIFY)
		return;

	opdu = g_attrib_get_buffer(attrib, &plen);
	olen = enc_confirmation(opdu, plen);

	if (olen > 0)
		g_attrib_send(attrib, 0, opdu, olen, NULL, NULL, NULL);
}
static gboolean check_nval(GIOChannel *io)
{
	struct pollfd fds;

	memset(&fds, 0, sizeof(fds));
	fds.fd = g_io_channel_unix_get_fd(io);
	fds.events = POLLNVAL;

	if (poll(&fds, 1, 0) > 0 && (fds.revents & POLLNVAL))
		return TRUE;

	return FALSE;
}
static gboolean connect_cb2(GIOChannel *io, GIOCondition cond,
							gpointer user_data)
{

	struct connect *conn = user_data;
	GError *gerr = NULL;
	int err, sk_err, sock;
	socklen_t len = sizeof(sk_err);

	/* If the user aborted this connect attempt */
	if ((cond & G_IO_NVAL) || check_nval(io))
		return FALSE;

	sock = g_io_channel_unix_get_fd(io);

	if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &sk_err, &len) < 0)
	{
			err = -errno;
				 g_print ("error1\n");
	}
	else
	{
		err = -sk_err;
			 g_print ("error2\n");
	}


	if (err < 0)
	{
			g_print ("err = %d\n",err);
			ERROR_FAILED(&gerr, "connect error", -err);
	}


	conn->connect(io, gerr, conn->user_data);
	 g_print ("timeout_callback called 1 times\n");
	g_clear_error(&gerr);

	return FALSE;
}

static void connect_remove(struct connect *conn)
{
	if (conn->destroy)
		conn->destroy(conn->user_data);
	g_free(conn);
}

static void connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
	g_print ("timeout_callback called 2 times\n");


	if (err) {
		//set_state(STATE_DISCONNECTED);


		//error("%s\n", err->message);
		return;
	}
	attrib = g_attrib_new(io);
	g_attrib_register(attrib, ATT_OP_HANDLE_NOTIFY, GATTRIB_ALL_HANDLES,
					events_handler, attrib, NULL);
	g_attrib_register(attrib, ATT_OP_HANDLE_IND, GATTRIB_ALL_HANDLES,
					events_handler, attrib, NULL);

	gatt_discover_primary(attrib, NULL, primary_all_cb, NULL);
	gatt_discover_primary(attrib, NULL, primary_all_cb, NULL);
	g_print("Connection successful\n");

}
static void connect_add(GIOChannel *io, BtIOConnect connect,
				gpointer user_data, GDestroyNotify destroy)
{
	struct connect *conn;
	GIOCondition cond;

	conn = g_new0(struct connect, 1);
	conn->connect = connect;
	conn->user_data = user_data;
	conn->destroy = destroy;

	cond = G_IO_OUT | G_IO_ERR | G_IO_HUP | G_IO_NVAL ;
	g_io_add_watch_full(io, G_PRIORITY_DEFAULT, cond, connect_cb2, conn,
					(GDestroyNotify) connect_remove);
}
static int l2cap_set_lm(int sock, int level)
{
	int lm_map[] = {
		0,
		L2CAP_LM_AUTH,
		L2CAP_LM_AUTH | L2CAP_LM_ENCRYPT,
		L2CAP_LM_AUTH | L2CAP_LM_ENCRYPT | L2CAP_LM_SECURE,
	}, opt = lm_map[level];

	if (setsockopt(sock, SOL_L2CAP, L2CAP_LM, &opt, sizeof(opt)) < 0)
		return -errno;

	return 0;
}
static gboolean set_sec_level(int sock, BtIOType type, int level, GError **err)
{
	struct bt_security sec;
	int ret;

	if (level < BT_SECURITY_LOW || level > BT_SECURITY_HIGH) {
		g_set_error(err, BT_IO_ERROR, EINVAL,
				"Valid security level range is %d-%d",
				BT_SECURITY_LOW, BT_SECURITY_HIGH);
		return FALSE;
	}

	memset(&sec, 0, sizeof(sec));
	sec.level = level;

	if (setsockopt(sock, SOL_BLUETOOTH, BT_SECURITY, &sec,
							sizeof(sec)) == 0)
		return TRUE;

	if (errno != ENOPROTOOPT) {
	//	ERROR_FAILED(err, "setsockopt(BT_SECURITY)", errno);
		return FALSE;
	}

	if (type == BT_IO_L2CAP)
		ret = l2cap_set_lm(sock, level);
	else
	//	ret = rfcomm_set_lm(sock, level);

	if (ret < 0) {
	//	ERROR_FAILED(err, "setsockopt(LM)", -ret);
		return FALSE;
	}

	return TRUE;
}


static int l2cap_bind(int sock, const bdaddr_t *src, uint8_t src_type,
				uint16_t psm, uint16_t cid, GError **err)
{
	struct sockaddr_l2 addr;

	memset(&addr, 0, sizeof(addr));
	addr.l2_family = AF_BLUETOOTH;
	bacpy(&addr.l2_bdaddr, src);


	addr.l2_cid = htobs(cid);


	addr.l2_bdaddr_type = src_type;

	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		int error = -errno;
	//	ERROR_FAILED(err, "l2cap_bind", errno);
		return error;
	}

	return 0;
}

static int l2cap_connect(int sock, const bdaddr_t *dst, uint8_t dst_type,
						uint16_t psm, uint16_t cid)
{
	int err;
	char ad[25];
	struct sockaddr_l2 addr;
    bdaddr_t *truedst;
   // str2ba(opt_dst,&truedst);
	memset(&addr, 0, sizeof(addr));

	addr.l2_family = AF_BLUETOOTH;
	bacpy(&addr.l2_bdaddr, dst);
	ba2str(dst,ad);
	printf("addr=%s\n",ad);

	if (cid)
	{
		addr.l2_cid = htobs(cid);
		printf("cid\n");
	}
	else
		addr.l2_psm = htobs(psm);

	addr.l2_bdaddr_type = dst_type;
	printf("dst_type=%d\n",dst_type);
	err = connect(sock, (struct sockaddr *) &addr, sizeof(addr));
	if (err < 0 && !(errno == EAGAIN || errno == EINPROGRESS))
		return -errno;

	return 0;
}


static gboolean l2cap_set(int sock, int sec_level, uint16_t imtu,
				uint16_t omtu, uint8_t mode, int master,
				int flushable, uint32_t priority, GError **err)
{
	if (imtu || omtu || mode) {
		struct l2cap_options l2o;
		socklen_t len;

		memset(&l2o, 0, sizeof(l2o));
		len = sizeof(l2o);
		if (getsockopt(sock, SOL_L2CAP, L2CAP_OPTIONS, &l2o,
								&len) < 0) {
			//ERROR_FAILED(err, "getsockopt(L2CAP_OPTIONS)", errno);
			return FALSE;
		}

		if (imtu)
			l2o.imtu = imtu;
		if (omtu)
			l2o.omtu = omtu;
		if (mode)
			l2o.mode = mode;

		if (setsockopt(sock, SOL_L2CAP, L2CAP_OPTIONS, &l2o,
							sizeof(l2o)) < 0) {
		//	ERROR_FAILED(err, "setsockopt(L2CAP_OPTIONS)", errno);
			return FALSE;
		}
	}



	if (sec_level && !set_sec_level(sock, BT_IO_L2CAP, sec_level, err))
		return FALSE;

	return TRUE;
}
int main()
{
	
		GError **err;
		bdaddr_t *sba, *dba;
		struct set_opts *opts;
		opts = (struct set_opts*)malloc(sizeof(struct set_opts));
		BtIOSecLevel sec = BT_IO_SEC_LOW;
		opts->defer = DEFAULT_DEFER_TIMEOUT;
		opts->master = -1;
		opts->mode = L2CAP_MODE_BASIC;
		opts->flushable = -1;
		opts->priority = 0;
		opts->src_type = BDADDR_LE_PUBLIC;
		str2ba("D8:AD:4A:AA:42:B5",&opts->dst);
		//str2ba("00:1A:7D:DA:71:13",&opts->src);
		opts->dst_type = BDADDR_LE_RANDOM;
		opts->type = BT_IO_L2CAP;
		opts->cid = ATT_CID;
		opts->sec_level = sec;

	 	GMainLoop *event_loop = g_main_loop_new(NULL, FALSE);


		int sock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
		GIOChannel *io = g_io_channel_unix_new(sock);
		g_io_channel_set_close_on_unref(io, TRUE);
		g_io_channel_set_flags(io, G_IO_FLAG_NONBLOCK, NULL);

		printf("sock=%d\n",sock);
		printf("binding...\n");
		int bind = l2cap_bind(sock, &opts->src, opts->src_type,
			0, opts->cid, err);
		printf("bind=%d\n",bind);
		printf("setting...\n");

		int set = l2cap_set(sock, opts->sec_level, opts->imtu, opts->omtu,
			opts->mode, opts->master, opts->flushable,
			opts->priority, err);
		printf("set=%d\n",set);
		printf("connecting....\n");
		int con = l2cap_connect( g_io_channel_unix_get_fd(io), &opts->dst, opts->dst_type,
						opts->psm, opts->cid);

		printf("connection=%d\n",con);

		connect_add(io, connect_cb, NULL, NULL);
	//	ERROR_FAILED(err, "connect", -con);

		g_main_loop_run(event_loop);
		g_main_loop_unref(event_loop);




		scanf("%d",&con);
		return 0;
}
//`pkg-config --cflags --libs glib-2.0`



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <unistd.h>

//#include </root/Desktop/libgatt/src/bluetooth.h>
//#include </root/Desktop/libgatt/src/hci.h>
//#include </root/Desktop/libgatt/src/hci_lib.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <bluetooth/uuid.h>
//#include </root/Desktop/libgatt/src/gatt.h>
//#include </root/Desktop/libgatt/src/btio.h>
//#include </root/Desktop/libgatt/src/gattrib.h>
#include <gatt/gatt.h>
#include <gatt/gattrib.h>
#include <gatt/btio.h>
static char *opt_src = "00:1A:7D:DA:71:13";
static char *opt_dst = "EE:52:F8:A2:89:1B";
//static char *opt_dst = "D8:AD:4A:AA:42:B5";
static char *opt_dst_type = NULL;
//static char *opt_value = NULL;
static char *opt_sec_level = NULL;
//static bt_uuid_t *opt_uuid = NULL;
//static int opt_start = 0x0001;
//static int opt_end = 0xffff;
//static int opt_handle = -1;
static int opt_mtu = 23;
static int opt_psm = 0;
//static gboolean opt_primary = FALSE;
//static gboolean opt_characteristics = FALSE;
//static gboolean opt_char_read = FALSE;
static gboolean opt_listen = FALSE;
//static gboolean opt_char_desc = FALSE;
//static gboolean opt_char_write = FALSE;
//static gboolean opt_char_write_req = FALSE;
//static gboolean opt_interactive = FALSE;
static GMainLoop *event_loop;
static gboolean got_error = FALSE;
static GSourceFunc operation;
static void events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data)
{
	GAttrib *attrib = user_data;
	uint8_t *opdu;
	uint16_t handle, i, olen = 0;
	size_t plen;

	handle = att_get_u16(&pdu[1]);

	switch (pdu[0]) {
	case ATT_OP_HANDLE_NOTIFY:
		g_print("Notification handle = 0x%04x value: ", handle);
		break;
	case ATT_OP_HANDLE_IND:
		g_print("Indication   handle = 0x%04x value: ", handle);
		break;
	default:
		g_print("Invalid opcode\n");
		return;
	}

	for (i = 3; i < len; i++)
		g_print("%02x ", pdu[i]);

	g_print("\n");

	if (pdu[0] == ATT_OP_HANDLE_NOTIFY)
		return;

	opdu = g_attrib_get_buffer(attrib, &plen);
	olen = enc_confirmation(opdu, plen);

	if (olen > 0)
		g_attrib_send(attrib, 0, opdu, olen, NULL, NULL, NULL);
}
static gboolean listen_start(gpointer user_data)
{
	GAttrib *attrib = user_data;

	g_attrib_register(attrib, ATT_OP_HANDLE_NOTIFY, GATTRIB_ALL_HANDLES,
						events_handler, attrib, NULL);
	g_attrib_register(attrib, ATT_OP_HANDLE_IND, GATTRIB_ALL_HANDLES,
						events_handler, attrib, NULL);

	return FALSE;
}

static void connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
	GAttrib *attrib;

	if (err) {
		g_printerr("%s\n", err->message);
		got_error = TRUE;
		g_main_loop_quit(event_loop);
	}

	attrib = g_attrib_new(io);

	if (opt_listen)
		g_idle_add(listen_start, attrib);

	operation(attrib);
}
int main()

{
	GIOChannel *chan;
	GError *gerr = NULL;
	opt_dst_type = g_strdup("random");
	opt_sec_level = g_strdup("low");
	chan = gatt_connect(opt_src, opt_dst, opt_dst_type, opt_sec_level,
				opt_psm, opt_mtu, connect_cb, &gerr);

	printf("p=%d\n",chan);
	//primary();
	//g_printerr("%s\n", gerr->message);

	g_io_channel_shutdown(chan, FALSE, NULL);
	g_io_channel_unref(chan);
	chan = NULL;
	return 0 ;
}

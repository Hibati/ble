

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>

#include <signal.h>
#include <sys/signalfd.h>
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
//static char *opt_dst = "EE:2A:81:DE:72:6A";
static char *opt_dst = "D8:AD:4A:AA:42:B5";
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
//static GAttrib *attrib = NULL;
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
static void primary_all_cb(GSList *services, guint8 status, gpointer user_data)
{
	GSList *l;
printf("good\n");
	

	for (l = services; l; l = l->next) {
		struct gatt_primary *prim = l->data;
		g_print("attr handle = 0x%04x, end grp handle = 0x%04x "
			"uuid: %s\n", prim->range.start, prim->range.end, prim->uuid);
	}
}
static gboolean primary(gpointer user_data)
{
	int i;
GAttrib *attrib = user_data;
printf("attr=%d\n",attrib);

  
  i= gatt_discover_primary(attrib, NULL, primary_all_cb, NULL);
  
  printf("i=%d",i);


}
static void connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
		
	printf("ddd\n");
	GAttrib *attrib;

	attrib = g_attrib_new(io);
		
		
	printf("ddd\n");
	primary(attrib);
}





int main()

{
	system("hciconfig hcio down");	
	system("hciconfig hcio up");
	//GOptionContext *context;
	
	
	GIOChannel *chan ;
	bdaddr_t sba, dba;
	uint8_t dest_type;
	GError *tmp_err = NULL;
	BtIOSecLevel sec;
	GError *gerr = NULL;
	opt_dst_type = g_strdup("random");
	opt_sec_level = g_strdup("low");
	//operation = primary;
	str2ba(opt_src, &sba);
	dest_type = BDADDR_LE_RANDOM;
	str2ba(opt_dst, &dba);
	sec = BT_IO_SEC_LOW;
	printf("connect\n");
	chan = bt_io_connect(NULL, NULL, NULL, &tmp_err,
					BT_IO_OPT_SOURCE_BDADDR, &sba,
					BT_IO_OPT_SOURCE_TYPE, BDADDR_LE_PUBLIC,
					BT_IO_OPT_DEST_BDADDR, &dba,
					BT_IO_OPT_DEST_TYPE, dest_type,
					BT_IO_OPT_CID, ATT_CID,
					BT_IO_OPT_SEC_LEVEL, sec,
					BT_IO_OPT_INVALID);

	

	
	connect_cb(chan,gerr,0);
	printf("chan=%d\n",chan);
	//gatt_discover_primary(attrib, NULL, primary_all_cb, NULL);
	//primary();
	//g_printerr("%s\n", tmp_err->message);
	//operation = primary;
	//g_io_channel_shutdown(chan, FALSE, NULL);
	//g_io_channel_unref(chan);
	//chan = NULL;
g_printerr("%s\n", tmp_err->message);

	return 0 ;
}

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
    static char *opt_src = "00:1A:7D:DA:71:13";
    //static char *opt_dst = "EE:2A:81:DE:72:6A";
    static char *opt_dst = "D8:AD:4A:AA:42:B5";
    GError **err;
    bdaddr_t *sba, *dba;
    str2ba(opt_dst, &dba);
   // str2ba(opt_src, &sba);
    struct set_opts *opts;
    opts = (struct set_opts*)malloc(sizeof(struct set_opts));
    BtIOSecLevel sec = BT_IO_SEC_LOW;
    //default settings
    opts->type = BT_IO_SCO;
	opts->defer = DEFAULT_DEFER_TIMEOUT;
	opts->master = -1;
	opts->mode = L2CAP_MODE_BASIC;
	opts->flushable = -1;
	opts->priority = 0;


	
	//bacpy(&sba, BDADDR_ANY);
   // bacpy(&opts->src, &sba);
    opts->src_type = BDADDR_LE_PUBLIC;

    str2ba("D8:AD:4A:AA:42:B5",&opts->dst);
     str2ba("00:1A:7D:DA:71:13",&opts->src);
    //bacpy(&opts->dst, &dba);

    char test[50];

    ba2str(&opts->dst,test);
    printf("opt->dst=%s\n",test);
    opts->dst_type = BDADDR_LE_RANDOM;

    opts->type = BT_IO_L2CAP;
	opts->cid = ATT_CID;
	
	opts->sec_level = sec;




   int sock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

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
     int con = l2cap_connect(sock, &opts->dst, opts->dst_type,
							opts->psm, opts->cid);

     printf("connection=%d\n",con);
     ERROR_FAILED(err, "connect", -con);
    return 0;
}

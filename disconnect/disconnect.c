#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>



int main()
{
	
	int dd = hci_open_dev(hci_get_route(NULL));
	int handle;

		               if (dd < 0) {
		                       perror("Could not open device");
		                       exit(1);
		               }
	printf("type the connection handle: ");
	scanf("%d",&handle);
	hci_disconnect(dd,handle , 0x16, 1000);
	return 0;
}

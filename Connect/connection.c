#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

int main(){
	

	
	//system("hciconfig hcio down");	
	system("hciconfig hcio up");

	//char bleaddr[16];

	//printf("Type the MAC Address: ");
	
//	scanf("%s",bleaddr);
	int  dd,err;
	        bdaddr_t bdaddr;
	        uint16_t interval, latency, max_ce_length, max_interval, min_ce_length;
	        uint16_t min_interval, supervision_timeout, window, handle;
	        uint8_t initiator_filter, own_bdaddr_type, peer_bdaddr_type;
	       
	       
	       
	dd = hci_open_dev(hci_get_route(NULL));

	     
	               if (dd < 0) {
	                       perror("Could not open device");
	                       exit(1);
	               }
	           //  str2ba(bleaddr,&bdaddr);
	               str2ba("D8:AD:4A:AA:42:B5", &bdaddr);


	                      interval = htobs(0x0004);
	                      window = htobs(0x0004);
	                      initiator_filter = 0x00;
	                      peer_bdaddr_type = 0x01;
	                      own_bdaddr_type = 0x00;
	                      min_interval = htobs(0x000F);
	                      max_interval = htobs(0x000F);
	                      latency = htobs(0x0000);
	                      supervision_timeout = htobs(0x0C80);
	                      min_ce_length = htobs(0x0001);
	                      max_ce_length = htobs(0x0001);

	                      err = hci_le_create_conn(dd, interval, window, initiator_filter,
	                                      peer_bdaddr_type, bdaddr, own_bdaddr_type, min_interval,
	                                      max_interval, latency, supervision_timeout,
	                                      min_ce_length, max_ce_length, &handle, 25000);
	                      if (err < 0) {
	                              perror("Could not create connection");
	                               hci_close_dev(dd);
	                              exit(1);
	                      }

	                      printf("Connection handle %d\n", handle);

	                      hci_close_dev(dd);
	                      
	                      
	                      return handle;
}

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <curses.h>
#define EIR_FLAGS                   0X01
#define EIR_NAME_SHORT              0x08
#define EIR_NAME_COMPLETE           0x09
#define EIR_MANUFACTURE_SPECIFIC    0xFF
struct Dev {
	char *name;
	char *add;
	struct Dev *next;
}dev;
struct Dev *root;
int size=0;
int IsNotDuplicate(char *addr)
{
	struct Dev *index =  root;
	int i=0;
	for(i=0;i<size;i++)
	{
		if(!strcmp( index->add,addr))
		return 0;
		
		index = index->next;
	}
	return 1;
}
struct Dev * nextdev(struct Dev *curr)
{
	return curr->next;
}
struct Dev * taildev(struct Dev *curr)
{
	struct Dev * index = curr;
	while(nextdev(index)!=0)
	{
		index=index->next;
	}
	return index;
}
void save_data(char *devname, char *addr)
{
	if(IsNotDuplicate(addr))
	{
		struct Dev *newitem =   (struct Dev *) malloc(sizeof(struct Dev));
		newitem->name =  (char *)malloc(50*sizeof(char));
		newitem->add =  (char *)malloc(16*sizeof(char));
		strcpy(newitem->name,devname);
		strcpy(newitem->add,addr);
		if(size==0)
		root = newitem ;
		else
		{
			taildev(root)->next= newitem ;
		}
		size++;
	}
	
}
void process_data(uint8_t *data, size_t data_len, le_advertising_info *info)
{

  if(data[0] == EIR_NAME_SHORT || data[0] == EIR_NAME_COMPLETE)
  {
    size_t name_len = data_len - 1;
    char *name = malloc(name_len + 1);
    memset(name, 0, name_len + 1);
    memcpy(name, &data[1], name_len);
    char addr[18];
    //printf("addr=%s",info.bdaddr[0]);
    ba2str(&info->bdaddr, addr);
    save_data(name,addr);
    free(name);
  }
  
 
 
  
    
}


void PrintAllItem()
{
	struct Dev *index = root;
	while(index!=NULL)
	{
		printf("Device name = %s   Address = %s\n",index->name,index->add);
		index= index->next;
	}
}

 void DeleteALLItem()
 {
	 struct Dev *temp=root;
	 struct Dev *next;
	 
	 
	int i=0;
	for(i=0;i<size;i++)
	{
		next = temp->next;
		free(temp->name);
		free(temp->add);
		free(temp);
		if(next==0)
		break;
		temp = next;
	}
	
		
	 
 }
 
int main(){
	system("hciconfig hcio down");	
	system("hciconfig hcio up");
	int dev_id = hci_get_route(NULL); // get the current USB adapter id
	int sock = hci_open_dev(dev_id); // connection to the microcontroller on the specified local Bluetooth adapter
	if(sock==0)
	return -1;
	int on = 1;
	ioctl(sock, FIONBIO, (char *)&on); //  nonblocking mode
	hci_le_set_scan_parameters(sock, 0x01, htobs(0x0010), htobs(0x0010), 0x00, 0x00, 1000);
	hci_le_set_scan_enable(sock, 0x01, 1, 1000);
	struct hci_filter old_filter;
	socklen_t olen = sizeof(old_filter);
	getsockopt(sock, SOL_HCI, HCI_FILTER, &old_filter,&olen) ;
	struct hci_filter new_filter;
	hci_filter_clear(&new_filter);
	hci_filter_set_ptype(HCI_EVENT_PKT, &new_filter);
	hci_filter_set_event(EVT_LE_META_EVENT, &new_filter);
	setsockopt(sock, SOL_HCI, HCI_FILTER, &new_filter, sizeof(new_filter));
	int done = 0;
	int error = 0;
	int counter = 0;
	
	while(!done && !error)
	  {
		  counter++;
				usleep(10);
		  if((counter%5)==0)
		  {
			  system("clear");
			  printf("scaning.....%.1f%% \n",(float)counter);
		}
		 
		if(counter==100)
		done=1;
	    int len = 0;
	    unsigned char buf[HCI_MAX_EVENT_SIZE];
	    while((len = read(sock, buf, sizeof(buf))) < 0)
	    {
			
			//printf("scaning....\n");
	      if (errno == EINTR)
	      {
	        done = 1;
	        break;
	      }
			if (errno == EAGAIN || errno == EINTR)
			{
			//	printf("no device........\n");
				
				usleep(100);
				continue;
			}

	      
	      error = 1;
	    }
		
	    if(!done && !error)
	    {
			
	      evt_le_meta_event *meta = (void *)(buf + (1 + HCI_EVENT_HDR_SIZE));
	      
	      len -= (1 + HCI_EVENT_HDR_SIZE);
	     // printf("len=%d\n",meta->subevent);

	      if (meta->subevent != EVT_LE_ADVERTISING_REPORT)
	      {
			  
			  printf("eve=%d",meta->subevent );
	        continue;
	      }

	      le_advertising_info *info = (le_advertising_info *) (meta->data + 1);


	      if(info->length == 0)
	      {
	        continue;
	      }

	      int current_index = 0;
	      int data_error = 0;

	      while(!data_error && current_index < info->length)
	      {
	        size_t data_len = info->data[current_index];

	        if(data_len + 1 > info->length)
	        {

	          data_error = 1;
	        }
	        else
	        {
			
				
	          process_data(info->data + current_index + 1, data_len, info);

	          current_index += data_len + 1;
	        }
	      }
	    }
	  }

	  if(error)
	  {

	  }
	  
	  
setsockopt(sock, SOL_HCI, HCI_FILTER, &old_filter, sizeof(old_filter));
hci_close_dev(sock);
 
 PrintAllItem();

 DeleteALLItem();

	return 0;
}

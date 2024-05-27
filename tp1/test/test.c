
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#define CAN_EFF_FLAG   0x80000000U 

int main(int argc, char **argv)
{
    // Global variables
	int s,i; 
	struct sockaddr_can addr;
	struct ifreq ifr;	
    int nbytes;
	struct can_frame frame, frame2;
    struct can_filter rfilter[1];
	frame.can_id = 0x8123 | CAN_EFF_FLAG; // This permit the use of an extended dataframe for the sender
	frame.can_dlc = 8; // Data lenght set to 8
	frame2.can_dlc = 8;
    frame.data[0] = 26; // Tirst octet is set to 26
	frame2.can_id = 0x8123; // Id is set to 8123 no need to use an extended dataframe
	rfilter[0].can_id  = 0x8123; // Id is set to 8123 for the filter
	rfilter[0].can_mask = 0xFF0; // Mask is set to 0xFF0

	printf("TEST demo\r\n");

    while(1)
    {
        // Catch errors
        if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) { // Socket errors
            perror("Socket");
            return 1;}

        // Definitions
        strcpy(ifr.ifr_name, "vcan0" ); //Copy vcan0 onto ifr.ifr_name
        ioctl(s, SIOCGIFINDEX, &ifr); // Configure network device
        memset(&addr, 0, sizeof(addr)); // Fill mem space
        addr.can_family = AF_CAN; // Define addr can family
        addr.can_ifindex = ifr.ifr_ifindex; // And index

        // Catch errors
        if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) { // Bind errors
            perror("Bind");
            return 1;}
        if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) { // Write errors
            perror("Write");
            return 1;}

        // Receiver
        if (frame2.can_id >= 0x100 && frame2.can_id <= 0x1FF) // Only if the id is between 0x100 and 0x1FF we print
        {
            printf("0x%03X [%d] ",frame2.can_id, frame2.can_dlc); // First we print the id and data lenght
            for (i = 0; i < frame2.can_dlc; i++)
            {
                printf("%d ",frame2.data[i]);// Then we print data
            }
        }
        printf("\r\n");

        // Sender
        setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));//Set socket options
        printf("0x%03X [%d] ",frame.can_id, frame.can_dlc); // We print for debug the id and dlc
        for (i = 0; i < frame.can_dlc; i++)
            printf("%02X ",frame.data[i]); // Print data
        printf("\r\n");
    }

    // Catch close errors
    if (close(s) < 0) {
		perror("Close");
		return 1;
	}
    
	return 0;
}

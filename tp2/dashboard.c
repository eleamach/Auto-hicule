
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

int motorspeed[1];
int vehiculespeed;
int geerselection;

void receivingData(){
    int s, i; 
	int nbytes;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;
    char a[3] = "^";
    int b, c, d, e;
    int valCaptRoad[5];
    int diffLeft, diffRight, diff;

     if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Socket");
		return 1;
	}

	strcpy(ifr.ifr_name, "vcan0" );
	ioctl(s, SIOCGIFINDEX, &ifr);

	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Bind");
		return 1;
	}

    while(1){
        nbytes = read(s, &frame, sizeof(struct can_frame));

        if (nbytes < 0) {
            perror("Read");
            return 1;
        }
        if (frame.can_id == 0x80000C06) 
        {
            b = frame.data[0];
            c = frame.data[1];
        }
        if (frame.can_id == 0x80000C07) 
        {
            d = frame.data[0];
            e = frame.data[1];
        }
        if (frame.can_id == 0x80000C00) {valCaptRoad[0]=frame.data[0];} 
        if (frame.can_id == 0x80000C01) {valCaptRoad[1]=frame.data[0];} 
        if (frame.can_id == 0x80000C02) {valCaptRoad[2]=frame.data[0];} 
        diffLeft = 3*valCaptRoad[0]+2*valCaptRoad[1]+valCaptRoad[2];
        if (frame.can_id == 0x80000C03) {valCaptRoad[3]=frame.data[0];} 
        if (frame.can_id == 0x80000C04) {valCaptRoad[4]=frame.data[0];} 
        if (frame.can_id == 0x80000C05) {valCaptRoad[5]=frame.data[0];} 
        diffRight = valCaptRoad[3]+2*valCaptRoad[4]+3*valCaptRoad[5];
        diff = diffLeft-diffRight;
        if (diff>15){strcpy(a, "<-");}
        else if (diff<=-15){strcpy(a, "->");}
        else {strcpy(a, "^");}
        printf("Speed: %d km/h\n", d);
        printf("Gear: %d\n", e);
        printf("Motor speed: %d%d rpm\n", b,c);
        printf("Action to follow the road : %s\n",a);
    }


	if (close(s) < 0) {
		perror("Close");
		return 1;
	}
                
}


int main()
{
    receivingData();  
	return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <signal.h>

int vehiculespeed;
int geerselection;


void envoieCAN (int ID, int dlc, unsigned char data[9]){
    int s; 
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame tramecan;
    struct can_filter rfilter[1];
	tramecan.can_id = ID; // This permit the use of an extended databeamblink for the sender
	tramecan.can_dlc = dlc; // Data lenght set to 8
	rfilter[0].can_id  = ID; // Id is set to 8123 for the filter
	rfilter[0].can_mask = 0xFF0; // Mask is set to 0xFF0

    // Catch errors
    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) { // Socket errors
        perror("Socket");
        return 1;}

    // Definitions
    strcpy(ifr.ifr_name, "vcan1" ); //Copy vcan0 onto ifr.ifr_name
    ioctl(s, SIOCGIFINDEX, &ifr); // Configure network device
    memset(&addr, 0, sizeof(addr)); // Fill mem space
    addr.can_family = AF_CAN; // Define addr can family
    addr.can_ifindex = ifr.ifr_ifindex; // And index

    tramecan.data[0]=data[0];
    tramecan.data[1]=data[1];
    tramecan.data[2]=data[2];
    tramecan.data[3]=data[3];
    tramecan.data[4]=data[4];
    tramecan.data[5]=data[5];
    tramecan.data[6]=data[6];
    tramecan.data[7]=data[7];
    tramecan.data[8]=data[8];

    // Catch errors
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) { // Bind errors
        perror("Bind");
        return 1;}
    if (write(s, &tramecan, sizeof(struct can_frame)) != sizeof(struct can_frame)) { // Write errors
        perror("Write");
        return 1;}
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));//Set socket options
    // Catch close errors
    if (close(s) < 0) {
		perror("Close");
		return 1;
	}
}



void receivingData(void){
    int s; 
	int nbytes;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;
    int motora, motorb, speed, wheel;

     if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Socket");
		return 1;
	}

	strcpy(ifr.ifr_name, "vcan1" );
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
        if (frame.can_id == 0x7E8) 
        {
            // CODE
            // Use variable speed, motora, motorb, wheel
            if (frame.data[2]==0x0D)
            {
                speed=frame.data[3];
                printf("Speed: %d km/h\n", speed);
            } 
            if (frame.data[2]==0x11)
            {
                wheel=frame.data[3];
                printf("Wheel : %d\n",wheel);
            } 
            if (frame.data[2]==0x0C)
            {
                motora=frame.data[3];
                motorb=frame.data[4];
                printf("Motor speed: %d%d rpm\n", motora,motorb);
            }      
        }
    }
	if (close(s) < 0) {
		perror("Close");
		return 1;
	}     
}


int main()
{    
    unsigned char request[8];
    request[0]=0x02;
    request[1]=0x01;
    request[3]=0xAA;
    request[4]=0xAA;
    request[5]=0xAA;
    request[6]=0xAA;
    request[7]=0xAA;
    pid_t pid;
    pid = fork();
    if (pid == -1) {
        perror("Erreur lors du fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { 
        while(1){
            request[2]=0x0D;
            envoieCAN(0x7DF, 8, request);
            request[2]=0x11;
            envoieCAN(0x7DF, 8, request);
            request[2]=0x0C;
            envoieCAN(0x7DF, 8, request); 
            sleep(1);
        }
    } else {
        // Processus parent : continuer l'exécution du reste du programme
        receivingData();  
    }

    return 0;
}

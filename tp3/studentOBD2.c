
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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY 1234

int motorspeed[1];
int vehiculespeed;
int geerselection;

struct SharedData {
    int motora;
    int motorb;
    int speed;
    int wheel;
};


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



void receivingData(struct SharedData *shared_data){
    int s; 
	int nbytes;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;

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
        nbytes = read(s, &frame, sizeof(struct can_frame));
        nbytes = read(s, &frame, sizeof(struct can_frame));
        nbytes = read(s, &frame, sizeof(struct can_frame));

        if (nbytes < 0) {
            perror("Read");
            return 1;
        }
        if (frame.can_id == 0x80000C06) 
        {
            shared_data->motora = frame.data[0];
            shared_data->motorb = frame.data[1];
        }
        if (frame.can_id == 0x80000C07) 
        {
            shared_data->speed = frame.data[0];
        }
        if (frame.can_id == 0x321) 
        {
            shared_data->wheel = frame.data[2];
        }
        
        printf("Speed: %d km/h\n", shared_data->speed);
        printf("Motor speed: %d%d rpm\n", shared_data->motora,shared_data->motorb);
        printf("Wheel : %d\n",shared_data->wheel);
    }
	if (close(s) < 0) {
		perror("Close");
		return 1;
	}     

}

void receivingDataVcan1(struct SharedData *shared_data){
    int s; 
	int nbytes;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;
    unsigned char response[8];

    response[0]=0x03;
    response[1]=0x41;
    response[5]=0xAA;
    response[6]=0xAA;
    response[7]=0xAA;

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
        if (frame.can_id == 0x7DF) 
        {
            if (frame.data[2]==0x0D)
            {
                response[2]=0x0D;
                response[3]=shared_data->speed;
                response[4]=0xAA;
            } 
            if (frame.data[2]==0x11)
            {
                response[2]=0x11;
                response[3]=shared_data->wheel;
                response[4]=0xAA;
            } 
            if (frame.data[2]==0x0C)
            {
                response[2]=0x0C;
                response[3]=shared_data->motora;
                response[4]=shared_data->motorb;
            } 
            envoieCAN(0x7E8, 8, response);
        }
    }
	if (close(s) < 0) {
		perror("Close");
		return 1;
	}     

}







int main()
{
    int shmid;
    if ((shmid = shmget(SHM_KEY, sizeof(struct SharedData), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    struct SharedData *shared_data;
    if ((shared_data = shmat(shmid, NULL, 0)) == (struct SharedData *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    pid_t pid;
    pid = fork();
    if (pid == -1) {
        perror("Erreur lors du fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { 
        receivingData(shared_data);  
    } else {
        receivingDataVcan1(shared_data);  
    }
    if (shmdt(shared_data) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }

    return 0;
}
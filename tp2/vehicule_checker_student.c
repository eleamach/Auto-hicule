
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

int debutprogramme; // Can only begin when receiving all data

void envoieCAN (int ID, int dlc, unsigned char data[7]){
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
    strcpy(ifr.ifr_name, "vcan0" ); //Copy vcan0 onto ifr.ifr_name
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

void receivingData(int ID, int dlc){
        // Global variables
        int s; 
        struct sockaddr_can addr;
        struct ifreq ifr;
        struct can_frame frame2;
        frame2.can_dlc = dlc;
        frame2.can_id = ID; // Id is set to 8123 no need to use an extended dataframe

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
        if (write(s, &frame2, sizeof(struct can_frame)) != sizeof(struct can_frame)) { // Write errors
            perror("Write");
            return 1;}
        // Receiver
        if (frame2.can_id >= 0xC00 && frame2.can_id <= 0xC07) // Only if the id is between 0xC00 and 0xC07 we print
        {
            debutprogramme += 1;    
        }
}



void controlCar(){
    // Global variables

    int s, nbytes;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;
    char a[3] = "^";
    int valCaptRoad[6];
    int diffLeft, diffRight;
    signed char diff;

    // Catch errors
     if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) { // socket errors
		perror("Socket");
		return 1;
	}

    // Definitions
	strcpy(ifr.ifr_name, "vcan0" );
	ioctl(s, SIOCGIFINDEX, &ifr);
	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

    // Catch errors
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {  // Bind errors
		perror("Bind");
		return 1;
	}

    // Definition of sent value for controling the car
    unsigned char car[7];
    car[0]=0x19;
    car[1]=0x00;
    printf("\nMove the car");
    envoieCAN(0x321, 3, car);

    while(1){
        // Read all frames
        nbytes = read(s, &frame, sizeof(struct can_frame));
        nbytes = read(s, &frame, sizeof(struct can_frame));
        nbytes = read(s, &frame, sizeof(struct can_frame));
        nbytes = read(s, &frame, sizeof(struct can_frame));
        nbytes = read(s, &frame, sizeof(struct can_frame));
        nbytes = read(s, &frame, sizeof(struct can_frame));

        // Catch errors
        if (nbytes < 0) { // Read errors
            perror("Read");
            return 1;
        }

        // Save frames value of sensors in our data
        if (frame.can_id == 0x80000C00) {valCaptRoad[0]=frame.data[0];} 
        if (frame.can_id == 0x80000C01) {valCaptRoad[1]=frame.data[0];} 
        if (frame.can_id == 0x80000C02) {valCaptRoad[2]=frame.data[0];} 
        if (frame.can_id == 0x80000C03) {valCaptRoad[3]=frame.data[0];} 
        if (frame.can_id == 0x80000C04) {valCaptRoad[4]=frame.data[0];} 
        if (frame.can_id == 0x80000C05) {valCaptRoad[5]=frame.data[0];} 

        // Calculate de difference between left and right
        diffLeft = 0.4*valCaptRoad[0]+0.3*valCaptRoad[1]+0.3*valCaptRoad[2];
        diffRight = 0.3*valCaptRoad[3]+0.3*valCaptRoad[4]+0.4*valCaptRoad[5];
        diff = diffLeft-diffRight;

        // To solve errors
        if (diff>100){diff=100;}
        if (diff<-100){diff=-100;}

        // Steer the wheel
        car[2]=(unsigned char)diff;
        envoieCAN(0x321, 3, car);
    
    // Speed limit
    if (frame.can_id == 0x80000C07) 
        {
            if (frame.data[0]>0x23) // If we exceed 50km/h we stop the throttle
            {
                car[0]=0x00;
            }
            else{
                car[0]=0x19; // Else we continue
            }
        }
    }

    // Catch errors
	if (close(s) < 0) { // Close errors
		perror("Close");
		return 1;
	}
                
}



int main()
{
    pid_t pid;
    pid = fork();
    if (pid == -1) {
        // Erreur lors de la création du processus fils
        perror("Erreur lors du fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Processus fils : lancer le simulateur
        if (execlp("python", "python", "-m", "avsim2D", NULL) == -1) {
            perror("Erreur lors du lancement du simulateur");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Test de la connexion au simulateur...");
        for (int i=1; i<=30; i++){
            receivingData(0xC00,7);
            if (debutprogramme == 7) {
                printf("\nDebut du programme\n");
                unsigned char accessories[7];
                // Blink
                accessories[1]=0x01;
                printf("\nRight blink");
                envoieCAN(0x123, 2, accessories);
                sleep(3);
                accessories[1]=0x02;
                printf("\nLeft blink");
                envoieCAN(0x123, 2, accessories);
                sleep(3);
                accessories[1]=0x00;
                // Beam
                accessories[0]=0x01;
                printf("\nLow beam");
                envoieCAN(0x123, 2, accessories);
                sleep(3);
                accessories[0]=0x02;
                printf("\nHigh beam");
                envoieCAN(0x123, 2, accessories);
                sleep(3);
                accessories[0]=0x00;
                printf("\nStop beam");
                envoieCAN(0x123, 2, accessories);
                sleep(1);
                controlCar();
                return 0;
            }
            printf("\n%d", i);
            sleep(2);
        }
        kill(pid, SIGTERM);
    }
    printf("\nConnexion impossible, merci de lancer le simulateur\n");
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

int debutprogramme; // Can only begin when receiving all data

int envoieCAN (int ID, int dlc, unsigned char data[8]){    
    int s=0; 
    struct can_filter rfilter[1];
	rfilter[0].can_id  = ID; // Id is set to 8123 for the filter
	rfilter[0].can_mask = 0xFF0; // Mask is set to 0xFF0
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame tramecan;
	tramecan.can_id = ID; // This permit the use of an extended databeamblink for the sender
	tramecan.can_dlc = dlc; // Data lenght set to 8
    int result=0;
    char if_name[IFNAMSIZ];  // Declare buffer with appropriate size


    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);    
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));//Set socket options
    if ((s < 0) || (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) || (write(s, &tramecan, sizeof(struct can_frame)) != sizeof(struct can_frame)) || (close(s) < 0))
    {
        perror("Error");
        result= 1; 
    }
    else {
        // Definitions
        strncpy(if_name, "vcan0", IFNAMSIZ);
        if_name[IFNAMSIZ - 1] = '\0'; // Ensure null-termination
        ioctl(s, SIOCGIFINDEX, &ifr); // Configure network device
        memset(&addr, 0, sizeof(addr)); // Fill mem space
        addr.can_family = AF_CAN; // Define addr can family
        addr.can_ifindex = ifr.ifr_ifindex; // And index    if (s < 0) { // Socket errors

        tramecan.data[0]=data[0];
        tramecan.data[1]=data[1];
        tramecan.data[2]=data[2];
        tramecan.data[3]=data[3];
        tramecan.data[4]=data[4];
        tramecan.data[5]=data[5];
        tramecan.data[6]=data[6];
        tramecan.data[7]=data[7];
    }
    return result;
}

int receivingData(int ID, int dlc){
    // Global variables
    int s=0; 
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame2; 
    frame2.can_dlc = dlc;
    frame2.can_id = ID; // Id is set to 8123 no need to use an extended dataframe
    int result=0;    
    char if_name[IFNAMSIZ];  // Declare buffer with appropriate size


    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if ((s < 0) || (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) || (write(s, &frame2, sizeof(struct can_frame)) != sizeof(struct can_frame)))
        {
            perror("Error");
            result= 1; 
        }
    else {
        // Definitions
        strncpy(if_name, "vcan0", IFNAMSIZ);
        if_name[IFNAMSIZ - 1] = '\0'; // Ensure null-termination
        ioctl(s, SIOCGIFINDEX, &ifr); // Configure network device
        unsigned char *ptr = (unsigned char *)&addr;
        for (size_t i = 0; i < sizeof(addr); ++i) {
            ptr[i] = 0;
        }        addr.can_family = AF_CAN; // Define addr can family
        addr.can_ifindex = ifr.ifr_ifindex; // And index

        // Receiver
        if ((frame2.can_id >= 0xC00) && (frame2.can_id <= 0xC07)) // Only if the id is between 0xC00 and 0xC07 we print
        {
            debutprogramme += 1;    
        }
    }
    return result;
}



int controlCar(void){
    // Global variables

    int s=0;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;
    int valCaptRoad[6];
    int diffLeft;
    int diffRight;
    signed char diff;
    int result=0;
    char if_name[IFNAMSIZ];  // Declare buffer with appropriate size


    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if ((s < 0) || (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) || (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) || (close(s) < 0))
        {
            perror("Error");
            result= 1; 
        }
    else {


        // Definitions
        strncpy(if_name, "vcan0", IFNAMSIZ);
        if_name[IFNAMSIZ - 1] = '\0'; // Ensure null-termination
        ioctl(s, SIOCGIFINDEX, &ifr);
        unsigned char *ptr = (unsigned char *)&addr;
        for (size_t i = 0; i < sizeof(addr); ++i) {
            ptr[i] = 0;
        }
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        // Definition of sent value for controling the car
        unsigned char car[8];
        car[0]=0x19;
        car[1]=0x00;

        envoieCAN(0x321, 3, car);

        while(1){
            // Read all frames
            read(s, &frame, sizeof(struct can_frame));
            read(s, &frame, sizeof(struct can_frame));
            read(s, &frame, sizeof(struct can_frame));
            read(s, &frame, sizeof(struct can_frame));
            read(s, &frame, sizeof(struct can_frame));
            read(s, &frame, sizeof(struct can_frame));

            // Save frames value of sensors in our data
            if (frame.can_id == 0x80000C00) {valCaptRoad[0]=frame.data[0];} 
            if (frame.can_id == 0x80000C01) {valCaptRoad[1]=frame.data[0];} 
            if (frame.can_id == 0x80000C02) {valCaptRoad[2]=frame.data[0];} 
            if (frame.can_id == 0x80000C03) {valCaptRoad[3]=frame.data[0];} 
            if (frame.can_id == 0x80000C04) {valCaptRoad[4]=frame.data[0];} 
            if (frame.can_id == 0x80000C05) {valCaptRoad[5]=frame.data[0];} 

            // Calculate de difference between left and right
            diffLeft = (0.4*valCaptRoad[0])+(0.3*valCaptRoad[1])+(0.3*valCaptRoad[2]);
            diffRight = (0.3*valCaptRoad[3])+(0.3*valCaptRoad[4])+(0.4*valCaptRoad[5]);
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
    }
    return result;
}



int main(void)
{
    char message[50] = "Test de la connexion au simulateur...";
    printf("%s", message);
    for (int i=1; i<=30; i++){
        receivingData(0xC00,7);
        if (debutprogramme == 7) {
            char message2[50] = "Debut de la simulation...";
            printf("%s", message2);
            unsigned char accessories[8];
            // Blink
            accessories[1]=0x01;
            envoieCAN(0x123, 2, accessories);
            sleep(3);
            accessories[1]=0x02;
            envoieCAN(0x123, 2, accessories);
            sleep(3);
            accessories[1]=0x00;
            // Beam
            accessories[0]=0x01;
            envoieCAN(0x123, 2, accessories);
            sleep(3);
            accessories[0]=0x02;
            envoieCAN(0x123, 2, accessories);
            sleep(3);
            accessories[0]=0x00;
            envoieCAN(0x123, 2, accessories);
            sleep(1);
            controlCar();
        }
        sleep(2);
    }
}

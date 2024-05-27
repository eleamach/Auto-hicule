
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


void envoieCANFD(int ID, int dlc, unsigned char data[54]) {
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct canfd_frame tramecan;
    int enable_canfd = 1;

    // Création de la socket
    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket");
        return;
    }

    // Activation des trames CAN FD
    if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_canfd, sizeof(enable_canfd)) < 0) {
        perror("Setsockopt");
        close(s);
        return;
    }

    // Configuration de l'interface
    strcpy(ifr.ifr_name, "vcan2"); 
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        perror("Ioctl");
        close(s);
        return;
    }

    // Configuration de l'adresse
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // Préparation de la trame CAN FD
    tramecan.can_id = ID;
    tramecan.len = dlc;
    memcpy(tramecan.data, data, dlc);

    // Envoi de la trame
    if (sendto(s, &tramecan, sizeof(tramecan), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Sendto");
        close(s);
        return;
    }

    // Fermeture de la socket
    close(s);
}




int receive() {
    int s, nbytes;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;
    unsigned char response[54]; // Utilisez unsigned char pour les données reçues
    int dataReceived = 0; // Utilisez un compteur pour suivre les données reçues

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket");
        return 1;
    }

    strcpy(ifr.ifr_name, "vcan0");
    ioctl(s, SIOCGIFINDEX, &ifr);
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        return 1;
    }

    while (1) {
        nbytes = read(s, &frame, sizeof(struct can_frame));
        if (nbytes < 0) {
            perror("Read");
            return 1;
        }

        // Copy received data to response array based on the frame ID
        if (frame.can_id >= 0x80000C00 && frame.can_id <= 0x80000C05) {
            int index = (frame.can_id & 0x0000000F) * 9; // Calculate index based on the frame ID
            for (int i = 0; i < 8; i++) {
                response[index + i] = frame.data[i];
            }
            dataReceived |= (1 << (frame.can_id - 0x80000C00)); // Mark the corresponding data as received
        }

        // Check if all data has been received
        if (dataReceived == 0x3F) { // 0x3F represents binary 111111 (all 6 sets of data received)
            envoieCANFD(0x100, 54, response); // Send the complete response
            dataReceived = 0; // Reset the data received flag
            memset(response, 0, sizeof(response)); // Clear response array for the next iteration
        }
    }

    close(s); // This line will never be reached in the current implementation
    return 0;
}



int main()
{
    receive();
    return 0;
}
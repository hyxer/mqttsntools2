/*
  MQTT-SN command-line publishing client
  Copyright (C) 2013 Nicholas Humfrey

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "mqtt-sn.h"

const char *client_id = NULL;
const char *topic_name = "test";
const char *message_data = "nama";
time_t keep_alive = 30;
const char *mqtt_sn_host = "127.0.0.1";
const char *mqtt_sn_port = "1884";
uint16_t topic_id = 0;
uint8_t topic_id_type = MQTT_SN_TOPIC_TYPE_NORMAL;
int8_t qos = 1;
uint8_t retain = FALSE;
uint8_t debug = TRUE;


static void usage()
{
    fprintf(stderr, "Usage: mqtt-sn-pub [opts] -t <topic> -m <message>\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -d             Enable debug messages.\n");
    fprintf(stderr, "  -h <host>      MQTT-SN host to connect to. Defaults to '%s'.\n", mqtt_sn_host);
    fprintf(stderr, "  -i <clientid>  ID to use for this client. Defaults to 'mqtt-sn-tools-' with process id.\n");
    fprintf(stderr, "  -m <message>   Message payload to send.\n");
    fprintf(stderr, "  -n             Send a null (zero length) message.\n");
    fprintf(stderr, "  -p <port>      Network port to connect to. Defaults to %s.\n", mqtt_sn_port);
    fprintf(stderr, "  -q <qos>       Quality of Service value (0 or -1). Defaults to %d.\n", qos);
    fprintf(stderr, "  -r             Message should be retained.\n");
    fprintf(stderr, "  -t <topic>     MQTT topic name to publish to.\n");
    fprintf(stderr, "  -T <topicid>   Pre-defined MQTT-SN topic ID to publish to.\n");
    exit(-1);
}

static void parse_opts(int argc, char** argv)
{
    int ch;

    // Parse the options/switches
    while ((ch = getopt(argc, argv, "dh:i:m:np:q:rt:T:?")) != -1)
        switch (ch) {
        case 'd':
            debug = TRUE;
        break;

        case 'h':
            mqtt_sn_host = optarg;
        break;

        case 'i':
            client_id = optarg;
        break;

        case 'm':
            message_data = optarg;
        break;

        case 'n':
            message_data = "";
        break;

        case 'p':
            mqtt_sn_port = optarg;
        break;

        case 'q':
            qos = atoi(optarg);
        break;

        case 'r':
            retain = TRUE;
        break;

        case 't':
            topic_name = optarg;
        break;

        case 'T':
            topic_id = atoi(optarg);
        break;

        case '?':
        default:
            usage();
        break;
    }

    // Missing Parameter?
    if (!(topic_name || topic_id) || !message_data) {
        usage();
    }

    //if (qos != -1 && qos != 0) {
    //    fprintf(stderr, "Error: only QoS level 0 or -1 is supported.\n");
    //    exit(-1);
    //}

    // Both topic name and topic id?
    if (topic_name && topic_id) {
        fprintf(stderr, "Error: please provide either a topic id or a topic name, not both.\n");
        exit(-1);
    }

    // Check topic is valid for QoS level -1
    if (qos == -1 && topic_id == 0 && strlen(topic_name) != 2) {
        fprintf(stderr, "Error: either a pre-defined topic id or a short topic name must be given for QoS -1.\n");
        exit(-1);
    }
}

int main(int argc, char* argv[])
{
    int sock;

    // Parse the command-line options
    parse_opts(argc, argv);

    // Enable debugging?
    //mqtt_sn_set_debug(debug);

    //variables to calculate time and write to file
    // clock_t begin, end;
    // double time_spent;
    // FILE * fp;
    //Start while 
    //while(i<100000){
    //begin = clock();  
    // Create a UDP socket
    sock = mqtt_sn_create_socket(mqtt_sn_host, mqtt_sn_port);
    if (sock) {
        // Connect to gateway
        if (qos >= 0) {
            mqtt_sn_send_connect(sock, client_id, keep_alive);
            mqtt_sn_receive_connack(sock);
        }

        if (topic_id) {
            // Use pre-defined topic ID
            topic_id_type = MQTT_SN_TOPIC_TYPE_PREDEFINED;
        } else if (strlen(topic_name) == 2) {
            // Convert the 2 character topic name into a 2 byte topic id
            topic_id = (topic_name[0] << 8) + topic_name[1];
            topic_id_type = MQTT_SN_TOPIC_TYPE_SHORT;
        } else if (qos >= 0) {
            // Register the topic name
            mqtt_sn_send_register(sock, topic_name);
            topic_id = mqtt_sn_receive_regack(sock);
            topic_id_type = MQTT_SN_TOPIC_TYPE_NORMAL;
        }


        // Publish to the topic
        if (qos == 0){
        mqtt_sn_send_publish(sock, topic_id, topic_id_type, message_data, qos, retain);
        }
        else{

        //begin = gettimeofday(); 
        
        // Manage Received Packets(If Any) - harith
        if (qos >= 1){
        mqtt_sn_send_publish(sock, topic_id, topic_id_type, message_data, qos, retain);
        
            if (qos == 1){

                struct timeval tv;
                fd_set rfd;
                int ret;
                int resend = 0;

                while (resend < 5){

                FD_ZERO(&rfd);
                FD_SET(sock, &rfd);

                tv.tv_sec = 1;
                tv.tv_usec = 0;

                ret = select(FD_SETSIZE, &rfd, NULL, NULL, &tv);
                    if (ret < 0) {
                        printf("Select() Error!\n" );
                        exit(EXIT_FAILURE);
                    }
                    else if (ret > 0) {

                        // Receive a packet
                        mqtt_sn_receive_puback(sock);
                        //printf("PUBACK Received!\n");
                        resend = 6;
                    }
                    else if (ret == 0)
                    {
                        printf("republishing...\n");
                        mqtt_sn_send_publish(sock, topic_id, topic_id_type, message_data, qos, retain);
                        resend++;
                    }
                }
            
            }
            //QOS=2
            else if(qos==2){
            mqtt_sn_receive_pubrec(sock);
            mqtt_sn_send_pubrel(sock);
            mqtt_sn_receive_pubcomp(sock);
            }
        }
    }
//      if (packet == "")
//          {
//              printf("The packet length is %2.2x\n",packet->length);
//          }
//              printf("The packet type is %2.2x\n",packet->type);
//              printf("The packet topic_id is %2.2x\n",packet->topic_id);
//              printf("The packet message_id is %2.2x\n",packet->message_id);
//              printf("The packet return code is %2.2x\n",packet->return_code);
//      else
//          {printf("Sending failed\n");}       
        // end = gettimeofday(); 
        // time_spent = (double)(end - begin);

        //open file for writing
        // begin = 0;
        // end   = 0; 
        // fp = fopen( "logfile.csv", "a" ); // Open file for writing
        // fprintf(fp, "Seq %d,%f\r\n",i,time_spent);

        // fclose(fp);
        
        


        if (qos >= 0) {
        mqtt_sn_send_disconnect(sock);
        
        //close(sock);
        }

        mqtt_sn_cleanup();
    
}

           return 0;
}

// void mqqt_wait_qos_one(int sock,int timeout){
    
//     time_t now = time(NULL);
//     struct timeval tv;
//     fd_set rfd;
//     int ret;

//     FD_ZERO(&rfd);
//     FD_SET(sock, &rfd);

//     tv.tv_sec = timeout;
//     tv.tv_usec = 0;

//     ret = select(FD_SETSIZE, &rfd, NULL, NULL, &tv);
//         if (ret < 0) {
//             if (errno != EINTR) {
//             // Something is wrong.
//             perror("select");
//             exit(EXIT_FAILURE);
//             }
//         } 
//         else if (ret == 0)
//         {
//             printf("timeout!\n", );
//         }
//         else if (ret > 0) {

//             // Receive a packet
//             mqtt_sn_receive_puback(sock);
//             monitor = 1;
//         }
    

// }

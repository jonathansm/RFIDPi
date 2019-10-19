/**
 * reader.c
 * Jonathan Smith
 * MIT License
 *
 * Copyright (c) 2019 Jonathan Smith
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *
 * API for any wiegand protocol reader. The reader can be wired up directly to a
 * RaspberryPi. Data 0 (Green) goes to pin 17. Data 1 clock (White) goes to pin 18.
 *
 * This driver will listen for data from the reader. Once it has data it will
 * try and decode the binary bits. after successful decode it will display the
 * card information for debug perposes and will do a POST request to send the
 * information to a database. The POST request is in JSON format. Default POST
 * is sent to localhost on port 5000. This API POST call is based on my
 * rfidpi_rest_api.py that is part of this same project.
 *
 * This code is based on several different sources.
 * https://github.com/lixmk/Wiegotcha
 * https://www.bishopfox.com/resources/tools/rfid-hacking/attack-tools/
 * https://gist.github.com/hsiboy/9598741
 *
 * Dependencies
 * wiringPi http://wiringpi.com
 * libcurl https://curl.haxx.se
 */

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wiringPi.h>
#include <curl/curl.h>

#define D0_PIN 0
#define D1_PIN 1

#define JSONFORMAT "{\"binary_value\":\"%s\",\"hex_value\":\"%s\",\"facility_code\":\"%s\",\"unique_code\":\"%s\",\"proxmark\":\"%s\"}"

#define WIEGANDMAXDATA 100
#define WIEGANDTIMEOUT 5000000

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

static unsigned char wiegandData[WIEGANDMAXDATA];
static unsigned long wiegandBitCount;
static struct timespec wiegandBitTime;
unsigned char flagDone;
unsigned int weigand_counter;
volatile unsigned long facilityCode = 0;
volatile unsigned long uniqueCode = 0;
volatile unsigned long bitHolder1 = 0;
volatile unsigned long bitHolder2 = 0;
volatile unsigned long preamble = 0;
volatile unsigned long cardData = 0;
unsigned char binaryBits[26];

void data0Pulse(void) {
  wiegandBitCount++;
  flagDone = 0;

  if (wiegandBitCount < 23) {
    bitHolder1 = bitHolder1 << 1;
  } else {
    bitHolder2 = bitHolder2 << 1;
  }

  weigand_counter = WIEGANDTIMEOUT;
}

void data1Pulse(void) {
  wiegandData[wiegandBitCount] = 1;
  wiegandBitCount++;
  flagDone = 0;

  if (wiegandBitCount < 23) {
    bitHolder1 = bitHolder1 << 1;
    bitHolder1 |= 1;
  } else {
    bitHolder2 = bitHolder2 << 1;
    bitHolder2 |= 1;
  }

  weigand_counter = WIEGANDTIMEOUT;
}

void wiegandReset() {
  memset((void *)wiegandData, 0, WIEGANDMAXDATA);
  wiegandBitCount = 0;
}

int wiegandGetPendingBitCount() {
  struct timespec now, delta;
  clock_gettime(CLOCK_MONOTONIC, &now);
  delta.tv_sec = now.tv_sec - wiegandBitTime.tv_sec;
  delta.tv_nsec = now.tv_nsec - wiegandBitTime.tv_nsec;

  if ((delta.tv_sec > 1) || (delta.tv_nsec > WIEGANDTIMEOUT))
    return wiegandBitCount;

  return 0;
}

void getCardUniqueAndFacilityCode() {
  // Will put this in a switch statement so that it will be easy to expand to
  // other bit formats
  switch (wiegandBitCount) {
  // 26 bit format
  // facility code = bits 2 to 9
  case 26:
    for (int i = 1; i < 9; i++) {
      facilityCode <<= 1;
      facilityCode |= wiegandData[i];
    }
    // unique code = bits 10 to 23
    for (int i = 9; i < 25; i++) {
      uniqueCode <<= 1;
      uniqueCode |= wiegandData[i];
    }
    break;
  }
  return;
}

void getCardValues() {
  // Will put this in a switch statement so that it will be easy to expand to
  // other bit formats
  switch (wiegandBitCount) {
  case 26:
    // Example of full card value
    // |>   preamble   <| |>   Actual card value   <|
    // 000000100000000001  11111000100000100100111000

    for (int i = 19; i >= 0; i--) {
      if (i == 13 || i == 2) {
        bitWrite(preamble, i, 1); // Write preamble 1's to the 13th and 2nd bits
      } else if (i > 2) {
        bitWrite(preamble, i,
                 0); // Write preamble 0's to all other bits above 1
      } else {
        bitWrite(
            preamble, i,
            bitRead(
                bitHolder1,
                i + 20)); // Write remaining bits to preamble from bitHolder1
      }
      if (i < 20) {
        bitWrite(
            cardData, i + 4,
            bitRead(bitHolder1,
                    i)); // Write the remaining bits of bitHolder1 to cardData
      }
      if (i < 4) {
        bitWrite(
            cardData, i,
            bitRead(
                bitHolder2,
                i)); // Write the remaining bit of cardData with bitHolder2 bits
      }
    }
    break;
  }
  return;
}
/*
 * wiegandReadData is a simple, non-blocking method to retrieve the last code
 * processed by the API.
 * data : is a pointer to a block of memory where the decoded data will be
 * stored. dataMaxLen : is the maximum number of -bytes- that can be read and
 * stored in data. Result : returns the number of -bits- in the current message,
 * 0 if there is no data available to be read, or -1 if there was an error.
 * Notes : this function clears the read data when called. On subsequent calls,
 * without subsequent data, this will return 0.
 */

int wiegandReadData(void *data, int dataMaxLen) {
  if (wiegandGetPendingBitCount() > 0) {
    int bitCount = wiegandBitCount;
    memcpy(data, (void *)wiegandData,
           ((bitCount > dataMaxLen) ? dataMaxLen : bitCount));
    getCardUniqueAndFacilityCode();
    getCardValues();
    wiegandReset();
    return bitCount;
  }
  return 0;
}

void sendPOST(char* fc, char* uc, char* hex, char* prox, char* binary) {
    CURL *curl;
  CURLcode res;
 
  curl_global_init(CURL_GLOBAL_ALL);
 
  curl = curl_easy_init();
  if(curl) {
    struct curl_slist *hs=NULL;
    hs = curl_slist_append(hs, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5000/api/tag");

    int jsonStringLen = snprintf(NULL, 0, JSONFORMAT ,binary,hex,fc,uc,prox);
    char jsonString[jsonStringLen+1];
    snprintf(jsonString, jsonStringLen+1, JSONFORMAT ,binary,hex,fc,uc,prox);


    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString);
 
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
      printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
}

void main(void) {
  printf("Starting Reader Driver. Now listening for reader input\n");

  wiringPiSetup();
  pinMode(D0_PIN, INPUT);
  pinMode(D1_PIN, INPUT);

  wiringPiISR(D0_PIN, INT_EDGE_FALLING, data0Pulse);
  wiringPiISR(D1_PIN, INT_EDGE_FALLING, data1Pulse);
  while (1) {
    if (!flagDone) {
      if (--weigand_counter == 0) {
        flagDone = 1;
      }
    }
    // If our flagDone bit is set we have at least 26 bits then display the card
    // info and send the POST request
    if (wiegandBitCount >= 26 && flagDone) {
      int bitLen = wiegandGetPendingBitCount();
      if (bitLen == 0) {
        usleep(5000);
      } else {
        char data[100];
        bitLen = wiegandReadData((void *)data, 100);

        int bitPlaceCount = 0;
        for (int i = 1; i >= 0; i--) {
          if (bitRead(preamble, i) == 0) {
            binaryBits[bitPlaceCount] = '0';
          } else {
            binaryBits[bitPlaceCount] = '1';
          }
          bitPlaceCount++;
        }

        for (int i = 23; i >= 0; i--) {
          if (bitRead(cardData, i) == 0) {
            binaryBits[bitPlaceCount] = '0';
          } else {
            binaryBits[bitPlaceCount] = '1';
          }
          bitPlaceCount++;
        }

        binaryBits[bitPlaceCount] = '\0';

        char *bin = binaryBits;
        int hex = 0;
        do {
          int b = *bin=='1'?1:0;
          hex = (hex<<1)|b;
          bin++;
        } while (*bin);


        int bitLenStringLen = snprintf(NULL, 0, "%d", bitLen);
        int facilityCodeStringLen = snprintf(NULL, 0, "%d", facilityCode);
        int uniqueCodeStringLen = snprintf(NULL, 0, "%d", uniqueCode);
        int proxStringLen = snprintf(NULL, 0, "%x%06x", preamble, cardData);
        int hexStringLen = snprintf(NULL, 0, "%X", hex);

        char bitLenString[bitLenStringLen+1];
        char facilityCodeString[facilityCodeStringLen+1];
        char uniqueCodeString[uniqueCodeStringLen+1];
        char proxString[proxStringLen+1];
        char hexString[hexStringLen+1];

        snprintf(bitLenString, bitLenStringLen+1, "%d", bitLen);
        snprintf(facilityCodeString, facilityCodeStringLen+1, "%d", facilityCode);
        snprintf(uniqueCodeString, uniqueCodeStringLen+1, "%d", uniqueCode);
        snprintf(proxString, proxStringLen+1, "%x%06x", preamble, cardData);
        snprintf(hexString, hexStringLen+1, "%X", hex);

        printf("Bits:%s  ", bitLenString);
        printf("Facility:%s  ", facilityCodeString);
        printf("Unique:%s  ", uniqueCodeString);
        printf("Proxmark:%s  ", proxString);
        printf("Hex:%s  ", hexString);
        printf("Binary:%s", binaryBits);

        printf("\nSending data to server\n");
        sendPOST(facilityCodeString, uniqueCodeString, hexString, proxString,binaryBits);

        facilityCode = 0;
        uniqueCode = 0;
        bitHolder1 = 0;
        bitHolder2 = 0;
        preamble = 0;
        cardData = 0;
      }
    }
  }
}
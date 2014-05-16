/*  
 *  DDS AD9850 Controller for Raspberry Pi
 *
 *  Copyright (C) 2014 EverPi - everpi[at]tsar[dot]in
 *  blog.everpi.net 
 * 
 *  This file is part of dds_ad9850.
 *
 *  dds_ad9850 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  dds_ad9850 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with dds_ad9850.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define W_CLK "11"
#define FU_UD "25"
#define DATA "9"
#define RESET "8"

#define HIGH "1"
#define LOW "0"

#define HELP \
        "\n  \033[1mDDS AD9850 Controller by blog.everpi.net\n\n" \
        "\tUsage:  [option] [frequency in HZ] [phase]\n" \
        "\t         -o\tchip shutdown\n\n"\
        "\tPhase is optional, 31 Max.\033[0m\n\n"


#define _ERR(c) if(c == -1){ fprintf(stderr,"%s\n",strerror(errno)); return 1; }

int send_bits(char data);
int spread(double freq);

int gpio_export(char *gp){
	
	int fd = 0;
	int er = 0;

        fd = open("/sys/class/gpio/export", O_WRONLY);
        _ERR(fd);

        er = write(fd,gp,2);
        close(fd);
        _ERR(er);
	
	return 0;
}

int gpio_direction(char *gp, char *direction){
	
	int fd = 0;
	int er = 0;
	char gpio_path[64];
	
    snprintf(gpio_path,64,"/sys/class/gpio/gpio%s/direction",gp);

    do{

	    fd = open(gpio_path, O_WRONLY);
	    
        if(errno == ENOENT){
            gpio_export(gp);
            
        }else _ERR(fd);

    }while(fd == -1);
    
    usleep(100000);
    er = write(fd,direction,3);
	close(fd);
	_ERR(er);
	
	return 0;
}

int gpio_value(char *gp, char *value){
	
    int fd = 0;
	int er = 0;
	char gpio_path[64];
	
    snprintf(gpio_path,64,"/sys/class/gpio/gpio%s/value",gp);    
    fd = open(gpio_path, O_WRONLY);
	_ERR(fd);
   
    er = write(fd,value,1);
	close(fd);
    _ERR(er);
	
	return 0;	
}

int gpio_pulse(char *gp){
	
	gpio_value(gp,HIGH);
	gpio_value(gp,LOW);

	return 0;
}

int dds_frequency(double freq, unsigned char phase){

    int bytes = 4;
    int tuning_word = freq * (4294967296.0 / 125000000.0);

#ifdef DEBUG
    printf("tuning_word(int):%d\n",tuning_word);
#endif

    while(bytes){
        
        send_bits(tuning_word & 0xff);
        tuning_word = tuning_word >> 8;
        bytes--;    
    }
    send_bits(phase); // 8 bits restantes, 5 para phase, 1 power-down, 3 de testes do fabricante
    gpio_pulse(FU_UD);
    return 0;
    
}

int send_bits(char data){
    
    int bits = 8;

    while(bits){
        
        if(data & 1){
            gpio_value(DATA,HIGH);
        
        }else gpio_value(DATA,LOW);

        gpio_pulse(W_CLK);
        data = data >> 1;
        bits--;
    }
    return 0;
}

int main(int argc, char *argv[]){

	unsigned char phase = 0;
	int frequency = 0;

	/* visto que a exportação leva um tempo X, espera-se 100ms para que o 
	 * diretório seria criado. 
	 */

	//usleep(100000);	

	gpio_direction(W_CLK,"out");
	gpio_direction(FU_UD,"out");
	gpio_direction(DATA,"out");
	gpio_direction(RESET,"out");	
	
	gpio_value(W_CLK,LOW);	
	gpio_value(FU_UD,LOW);
	gpio_value(DATA,LOW);
	gpio_value(RESET,LOW);	
	
	gpio_pulse(RESET);
	gpio_pulse(W_CLK);
	gpio_pulse(FU_UD);	
	
    if(argc < 2){
    
        printf("%s",HELP);
        return 0;
    }

    if(argv[1][1] == 'o'){
		dds_frequency(0,0x4); // set power down bit 00000100
		return 0;
	}	
    
    if(argc > 3 && phase <= 31){
	    phase = strtol(argv[2],NULL,10);
    }
    
    frequency = strtol(argv[1],NULL,10);
	
    if(frequency <= 40000000){
        //spread(frequency);
		dds_frequency(frequency, phase << 3 );
	}else{
		printf("Frequency max:40000000\n");
		return -1;
	}

	return 0;	
	
}

int spread(double freq){
    
    int range = 500000; // in khz
    int v_range = 0;
    
    fprintf(stderr,"freqspread:%lf\n",freq);

    while(1){

        dds_frequency(freq+(double)v_range,0);
        
        usleep(1000);
        if(v_range > range) v_range = 0;
        v_range+=80000;
    }

    return 0;
}

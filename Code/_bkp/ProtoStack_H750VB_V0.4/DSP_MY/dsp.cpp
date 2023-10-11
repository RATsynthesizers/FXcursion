/*
 * dsp.cpp
 *
 *  Created on: Oct 22, 2020
 *      Author: romte
 */

#include "../DSP_MY/dsp.hpp"


//saturation addition
float satSubtract(float x, float y){            //x - y
    if(x < (FLOAT_MIN + y))                           //if((x - y) < MIN), add y to both sides to yields if(x < (MIN + y))
        return FLOAT_MIN;
    if(-y > (FLOAT_MAX - x))                          //if((x - y) > MAX), subtract x from both sides to yield if(-y > (MAX - x))
        return FLOAT_MAX;
    return (x - y);                             //if it's not limiting then just return x - y
}

//saturation subtraction
float satAdd(float x, float y){                 //x + y
    if(x < (FLOAT_MIN - y))                           //if((x + y) < MIN)
        return FLOAT_MIN;
    if(x > (FLOAT_MAX - y))                           //if((x + y) > MAX)
        return FLOAT_MAX;
    return (x + y);                             //if it's not limiting then just return x + y
}

//signed to unsigned integer conversion
float intToFloat(int input){
    uint16_t output = 0;
    float output_f = 0;
    if(0x8000 && input){
        output = input - 0x8000;                //then it's a negative number
    }else{
        output = input + 0x8000;                //then it's a postive number
    }
    output_f = output/2.0;
    output_f -= 32768.0/2.0;                    //remove DC offsset from signed to unsigned conversion.
    output_f *= float(2.0);                     //amplify a little.
    return output_f;
}

//unsigned to signed integer conversion
int unsignedToSigned(int input){
    int output = 0;
    if(input >= 0x8000)                         //then convert it to a positive signed numbber
        output = ((input >> 1) & 0x7FFF);
    if(input < 0x8000)
        output = input + 0x8000;
    return output + 32768/2;
}

//these ideas taken from paulbourke.net/miscellaneous/interpolation/
//linear
float linterp(float y1, float y2, float frac){
    return(y1*(1-frac) + y2*frac);
}
//cosine
float cosinterp(float y1, float y2, float frac){
    float frac2;
    frac2 = (1 - cos(frac*float(3.14159)))/2;
    return(y1*(1-frac2) + y2*frac2);
}


void OnePoleLp::setFc(float Fc) {
    b1 = exp(-2.0 * 3.14159 * Fc);
    a0 = float(1.0) - b1;
}
float OnePoleLp::process(float in) {
    return z1 = in * a0 + z1 * b1;
}



void OnePoleHp::setFc(float Fc) {
    b1 = -exp(-2.0 * 3.14159 * (float(0.5) - Fc));
    a0 = float(1.0) + b1;
}
float OnePoleHp::process(float in) {
    return z1 = in * a0 + z1 * b1;
}


float Delay::process(float in, float fbin, float length){
    //update index values
    index1++;
    index2++;
    if(index1 >= (DELAY_DELAYLEN))
        index1 = 0;
    if(index2 >= (DELAY_DELAYLEN))
        index2 = 0;

    //separate length into integer and fractional parts for interpolation
    intlength = length;
    fraclength = length - intlength;

    //update indices based on delay length
    index2 = index1 + intlength;
    if(index2 >= DELAY_DELAYLEN)
        index2 = index1 + intlength - DELAY_DELAYLEN;

    //write to delay buffer
    buffer[index1] = satAdd(in,fbin);

    //read the delays out of the delay array.  do some crazy interpolation stuff.
    if(index2+1 < DELAY_DELAYLEN){
        out = linterp(buffer[index2],buffer[index2+1],fraclength);    //worry about the converion from float to uint outside of this.
    }else{
        out = buffer[index2];                                                    //probably just want to take the output of the delay and not the dry.
    }
    return out;
}


//////////////////////// ALLPASS ////////////////////////
void AllPass::process(float in, float feedback, float length){
    //update index values
    index1++;
    index2++;
    if(index1 >= (ALLPASS_DELAYLEN))
        index1 = 0;
    if(index2 >= (ALLPASS_DELAYLEN))
        index2 = 0;

    //separate length into integer and fractional parts for interpolation //TODO length -- float
    intlength = length;
    fraclength = length - intlength;

    //update indices based on delay length
    index2 = index1 + intlength;
    if(index2 >= ALLPASS_DELAYLEN)
        index2 = index1 + intlength - ALLPASS_DELAYLEN;

    //write to delay buffer
    buffin = satAdd(in,buffout*feedback);
    buffer[index1] = buffin;

    //read the delays out of the delay array.  do some crazy interpolation stuff.
    if(index2+1 < ALLPASS_DELAYLEN){
        buffout = linterp(buffer[index2],buffer[index2+1],fraclength);    //worry about the converion from float to uint outside of this.
    }else{
        buffout = buffer[index2];                                         //probably just want to take the output of the delay and not the dry.
    }

    out = satSubtract(buffout,buffin*feedback);
}

void AllPass::process(float in) {
    //update index values
    index1++;
    index2++;
    if(index1 >= (ALLPASS_DELAYLEN))
        index1 = 0;
    if(index2 >= (ALLPASS_DELAYLEN))
        index2 = 0;

    //update indices based on delay length
    index2 = index1 + intlength;
    if(index2 >= ALLPASS_DELAYLEN)
        index2 = index1 + intlength - ALLPASS_DELAYLEN;

    //write to delay buffer
    buffin = satAdd(in,buffout*feedback);
    buffer[index1] = buffin;

    //read the delays out of the delay array.  do some crazy interpolation stuff.
    if(index2+1 < ALLPASS_DELAYLEN){
        buffout = linterp(buffer[index2],buffer[index2+1],fraclength);    //worry about the converion from float to uint outside of this.
    }else{
        buffout = buffer[index2];                                         //probably just want to take the output of the delay and not the dry.
    }

    out = satSubtract(buffout,buffin*feedback);
}

//////////////////////// ALLPASS_PSRAM ////////////////////////
void AllPass_PSRAM::process(float in, float feedback, float length){
    //update index values
    index1++;
    index2++;
    if(index1 >= (ALLPASS_DELAYLEN))
        index1 = 0;
    if(index2 >= (ALLPASS_DELAYLEN))
        index2 = 0;

    //separate length into integer and fractional parts for interpolation //TODO length -- float
    intlength = length;
    fraclength = length - intlength;

    //update indices based on delay length
    index2 = index1 + intlength;
    if(index2 >= ALLPASS_DELAYLEN)
        index2 = index1 + intlength - ALLPASS_DELAYLEN;

    //write to delay buffer
    buffin = satAdd(in,buffout*feedback);
    buffer_index1 = buffin;

    //read the delays out of the delay array.  do some crazy interpolation stuff.
    if(index2+1 < ALLPASS_DELAYLEN){
        buffout = linterp(buffer_index2,buffer_indexFrac,fraclength);    //worry about the converion from float to uint outside of this.
    }else{
        buffout = buffer_index2;                                         //probably just want to take the output of the delay and not the dry.
    }

    out = satSubtract(buffout,buffin*feedback);
}

void AllPass_PSRAM::process(float in){
    //update index values
    index1++;
    index2++;
    if(index1 >= (ALLPASS_DELAYLEN))
        index1 = 0;
    if(index2 >= (ALLPASS_DELAYLEN))
        index2 = 0;

    //update indices based on delay length
    index2 = index1 + intlength;
    if(index2 >= ALLPASS_DELAYLEN)
        index2 = index1 + intlength - ALLPASS_DELAYLEN;

    //write to delay buffer
    buffin = satAdd(in,buffout*feedback);
    buffer_index1 = buffin;

    //read the delays out of the delay array.  do some crazy interpolation stuff.
    if(index2+1 < ALLPASS_DELAYLEN){
        buffout = linterp(buffer_index2,buffer_indexFrac,fraclength);    //worry about the converion from float to uint outside of this.
    }else{
        buffout = buffer_index2;                                         //probably just want to take the output of the delay and not the dry.
    }

    out = satSubtract(buffout,buffin*feedback);
    // psram
    dma_rx_ready_flag = 2;
    dma_tx_ready_flag = 1; // flag to write old index1 sample and read old index2 & indexFrac
    				// in Audio Sys Objects
}

//////////// GORDON SMITH OSCILLATOR ///////////////////
void gsOsc::process(){
    yq = yq_1 - eps*yn_1;
    yn = eps*yq + yn_1;
    yn_1 = yn;
    yq_1 = yq;
    if(quad)
        out = yn*amp2;
    else
       out =  yq*amp2;
}

void gsOsc::setFA(float frequency, float amp, int quad){
	this->quad = quad;
    fw = 2.0f*pi*frequency/fs;
    eps = 2.0f*sin(fw/2.0f);
    amp2 = amp;
}

/*
 * dsp.hpp
 *
 *  Created on: Oct 22, 2020
 *      Author: romte
 */

#ifndef INC_DSP_HPP_
#define INC_DSP_HPP_

#include <math.h>
#include <stdint.h>
#include "Handy.h"

#include "../Modules/SysGlobals.h"

//Header including the saturating math functions.  Also includes signed to unsigned and unsigned to signed integer conversion
//functions.

#define F_PI ((float)M_PI)

#define FLOAT_MIN   -62000.0f                         // TODO ?? so you don't get negative vlaues because the stuff is in unsigned format
#define FLOAT_MAX   62000.0f                          //limit at maximum 16 bit value so it doesn't wrap around

#define DELAY_DELAYLEN     3000
#define ALLPASS_DELAYLEN   3000

#define _CLIP_LIMIT(x, d, u)	if((x)>(u)){(x)=(u);}else if((x)<(d)) {(x)=(d);}
#define _F2RAD(x)				(((x) / SAMPLINGFREQ) * 2*M_PI)  // Hz -> Rad per sample
#define _RAD2F(x)				(((x) / (2*M_PI)) * SAMPLINGFREQ)
#define _NORMF(f)				((f) / SAMPLINGFREQ)  // normalise freq to [0..1]
#define _MAP(x, in_min, in_max, out_min, out_max) 	(((x) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + (out_min))
#define _MAP01(x, y_min, y_max)						( x*(y_max-y_min) + y_min )


//saturation addition
float satSubtract(float x, float y);            //x - y
//saturation subtraction
float satAdd(float x, float y);                 //x + y
//signed to unsigned integer conversion
float intToFloat(int input);
//unsigned to signed integer conversion
int unsignedToSigned(int input);
//linear
float linterp(float y1, float y2, float frac);
//cosine
float cosinterp(float y1, float y2, float frac);

//1 pole lowpass filter from ear level engineering's website
class OnePoleLp {
public:
    OnePoleLp() {a0 = 1.0; b1 = 0.0; z1 = 0.0;}
    OnePoleLp(float Fc) {z1 = 0.0; setFc(Fc);}
    //~OnePole();
    void setFc(float Fc);
    float process(float in);
protected:
    float a0, b1, z1;
};

//1 pole highpass filter
class OnePoleHp {
public:
    OnePoleHp() {a0 = 1.0; b1 = 0.0; z1 = 0.0;}
    OnePoleHp(float Fc) {z1 = 0.0; setFc(Fc);}
    //~OnePole();
    void setFc(float Fc);
    float process(float in);
protected:
    float a0, b1, z1;
};

//make a delay class
class Delay {
public:
    Delay() {
        index1 = 0;
        index2 = DELAY_DELAYLEN*.99;
        for(int i = 0; i <= DELAY_DELAYLEN; i++)
            buffer[i] = 0;
        intlength = DELAY_DELAYLEN;
        out = 0;
        fraclength = 0;
    }
    void setLength(float length);
    float process(float in, float fbin, float length);
protected:
    float buffer[DELAY_DELAYLEN];
    int index1, index2, intlength;
    float fraclength, out;
};

//make an allpass class
class AllPass {
public:
	float out;

    AllPass() {
        index1 = 0;
        index2 = ALLPASS_DELAYLEN-1; // TODO ??
		for (int i = 0; i < ALLPASS_DELAYLEN; i++)
			buffer[i] = 0;
		intlength = 0;
		buffin = 0;
		buffout = 0;
		feedback = 0;
		fraclength = 0;
		out = 0;
    }
    AllPass(float length, float feedback) {
        index1 = 0;
        index2 = ALLPASS_DELAYLEN-1; // TODO ??
		for (int i = 0; i < ALLPASS_DELAYLEN; i++)
			buffer[i] = 0;
		buffin = 0;
		buffout = 0;
		out = 0;
		setLength(length);
		setFB(feedback);
    }
    void setLength(float length) { 	intlength = length; fraclength = length - intlength; }
    void setFB(float feedback) { this->feedback = feedback; }
    void process(float in, float feedback, float length);
    void process(float in);

protected:
    float buffer[ALLPASS_DELAYLEN];
    int index1, index2, intlength;
    float feedback, fraclength, buffout, buffin;
};

//make class for gordon smith oscillator
class gsOsc {
public:
	float out;

    gsOsc(){
        yn = 0;         //initial condition sine(0) = 0
        yq = 0;         //initial condition cos(0) = 1
        yn_1 = 1, yq_1 = 0;     //hmm...
        pi = 3.14159;
        fs = SAMPLINGFREQ;
        quad = 0;
        out = 0;

        fw = 0;
        eps = 0;
        amp2 = 0;
    }

    gsOsc(float frequency, float amp, int quad) {
        yn = 0;
        yq = 0;
        yn_1 = 1, yq_1 = 0;
        pi = 3.14159f;
        fs = (float)SAMPLINGFREQ;
        quad = 0;
        out = 0;
    	setFA(frequency, amp, quad);
    }

    void setFA(float frequency, float amp, int quad);
    void process();
protected:
    //put variables here that you'll use within process
    float yn, yq, yn_1, yq_1, fw, pi, fs, eps, amp2;
    int quad;
};

#endif /* INC_DSP_HPP_ */

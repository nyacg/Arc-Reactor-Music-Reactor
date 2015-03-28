#define N_PIXELS	 6	// Number of pixels in strand
#define MIC_PIN	 A1	// Microphone is attached to this analog pin
#define DC_OFFSET	0	// DC offset in mic signal - if unusure, leave 0
#define NOISE		 10	// Noise/hum/interference in mic signal
#define SAMPLES	 60	// Length of buffer for dynamic level adjustment
#define TOP			 (N_PIXELS + 1) // Allow dot to go slightly off scale
#define PEAK_FALL 40	// Rate of peak falling dot

byte
	peak			= 0,			// Used for falling dot
	dotCount	= 0,			// Frame counter for delaying dot-falling speed
	volCount	= 0;			// Frame counter for storing past volume data
int
	vol[SAMPLES],			 // Collection of prior volume samples
	lvl			 = 10,			// Current "dampened" audio level
	minLvlAvg = 0,			// For dynamic adjustment of graph low & high
	maxLvlAvg = 512,
	ON				= HIGH,
	OFF			 = LOW,
	data = 1,
	clock = 4,
	latch = 3,
	ledState = 0,
	numRows = 0,
	prevRows = 0,
	currentState = 0;
	
	void setup() {
		
	analogReference(EXTERNAL);	// ensure analong readings are compared to input level at REF pin
	
	// setup pins for shift register
	pinMode(data, OUTPUT);
	pinMode(clock, OUTPUT);	
	pinMode(latch, OUTPUT);
	
//	Serial.begin(9600);
	memset(vol, 0, sizeof(vol));
}


void loop() {
	numRows = readPeak()-1;
	
	updateRing();
	prevRows = numRows;
	if (prevRows < 0) prevRows = 0;
}



int readPeak(){
	// function adapted from adafruit ampli-tie project https://learn.adafruit.com/led-ampli-tie/overview
	uint8_t	i;
	uint16_t minLvl, maxLvl;
	int n, height;

	n = analogRead(MIC_PIN);
	n = abs(n - 512 - DC_OFFSET); // Center on zero
	
	n = (n <= NOISE) ? 0 : (n - NOISE); // Remove noise/hum
	lvl = ((lvl * 7) + n) >> 3; // "Dampened" reading (else looks twitchy)

	// Calculate bar height based on dynamic min/max levels (fixed point):
	height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

	if(height < 0L) height = 0;			// Clip output
	else if(height > TOP) height = TOP;
	if(height > peak) peak = height; // Keep 'peak' dot at top

// Every few frames, make the peak pixel drop by 1:

		if(++dotCount >= PEAK_FALL) { //fall rate 
			
			if(peak > 0) peak--;
			dotCount = 0;
		}

	vol[volCount] = n;											// Save sample for dynamic leveling
	if(++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter

	// Get volume range of prior frames
	minLvl = maxLvl = vol[0];
	for(i=1; i<SAMPLES; i++) {
		if(vol[i] < minLvl) minLvl = vol[i];
		else if(vol[i] > maxLvl) maxLvl = vol[i];
	}
	
	// minLvl and maxLvl indicate the volume range over prior frames, used
	// for vertically scaling the output graph (so it looks interesting
	// regardless of volume level).	If they're too close together though
	// (e.g. at very low volume levels) the graph becomes super coarse
	// and 'jumpy'...so keep some minimum distance between them (this
	// also lets the graph go to zero when no sound is playing):
 
		if((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
		minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
		maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
	
	return peak;
}

void updateRing(){
	if(numRows < 0){
		
	} else if(numRows > prevRows){
		for(int i=prevRows; i < numRows; i++){
		 changeLED(i, ON);
		 delay(20);
		}
	} else if(numRows < prevRows){
		for(int i=prevRows; i >= numRows; i--){
		 changeLED(i, OFF);
		 delay(10);
		} 
	}
}

// test function used to check if upload worked 
void onThenOff(){
	for(int i=0; i<6; i++){
		changeLED(i, ON);
		delay(100);
	}
	 for(int i=0; i<6; i++){
		changeLED(i, OFF);
		delay(100);
	}
}

void updateLEDs(int value){
	digitalWrite(latch, LOW);		 //Pulls the chips latch low
	shiftOut(data, clock, MSBFIRST, value); //Shifts out the 8 bits to the shift register
	digitalWrite(latch, HIGH);	 //Pulls the latch high displaying the data
}

//These are used in the bitwise math that we use to change individual LEDs
int bits[] = {B00000001, B00000010, B00000100, B00001000, B00010000, B00100000, B01000000, B10000000};
int masks[] = {B11111110, B11111101, B11111011, B11110111, B11101111, B11011111, B10111111, B01111111};
/*
 * changeLED(int led, int state) - changes an individual LED 
 * LEDs are 0 to 7 and state is either 0 - OFF or 1 - ON
 */
 void changeLED(int led, int state){
	ledState = ledState & masks[led];	//clears ledState of the bit we are addressing
	if(state == ON){ledState = ledState | bits[led];} //if the bit is on we will add it to ledState
	updateLEDs(ledState);			//send the new LED state to the shift register
 }

#include <Wire.h>
//#include <QMC5883LCompass.h>
#include <BME280I2C.h>
#include <TinyGPS.h>
#define M_PI 3.1415926535897931
#define __BCOMP_MAXRANGE__ 50001
#define GRAVITY (-32.194)
#define I2C_SDA 27
#define I2C_SCL 26

#define SEALEVELPRESSURE_HPA (1013.25)

double range;			// Range in yds
double path;			// Path in inches
double elevationMOA;	// Correction in MOA
double flightTime;		// Time in s
double windage;		 	// Windage in inches
double windageMOA;		// Windage in MOA
double velocityCombined;//Velocity (combined)
double velocityX;		// Velocity (x)
double velocityY;		// Velocity (y)
BME280I2C bme;			// Default : forced mode, standby time = 1000 ms
						// Oversampling = pressure �1, temperature �1, humidity �1, filter off,

//QMC5883LCompass compass;

enum DragFunction { G1 = 1, G2, G3, G4, G5, G6, G7, G8 };

unsigned long delayTime;

/*
* This application is urrently set up to interrogate a BME280 sensor and display the returned avalues to a serial monitor
*/

void setup()
{
	Serial.begin(115200);
	while (!Serial) {} // Wait
	Serial.println(F("BME280 test"));

	bool status;

#ifdef ESP32
	Wire.begin(I2C_SDA, I2C_SCL);
#else
	Wire.begin();
#endif

	while (!bme.begin())
	{
		Serial.println("Could not find BME280 sensor!");
		delay(1000);
	}

	switch (bme.chipModel())
	{
	case BME280::ChipModel_BME280:
		Serial.println("Found BME280 sensor! Success.");
		break;
	case BME280::ChipModel_BMP280:
		Serial.println("Found BMP280 sensor! No Humidity available.");
		break;
	default:
		Serial.println("Found UNKNOWN sensor! Error!");
	}

	//Wire.begin();
	// Initialize the Compass.
	//compass.init();

	Serial.println("-- Default Test --");
	delayTime = 1000;

	Serial.println();
}

void loop()
{
	//readCompass();
	printBME280Data(&Serial);
	delay(500);
}

void printBME280Data(Stream* client)
{
	float temp(NAN), hum(NAN), pres(NAN);

	BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
	BME280::PresUnit presUnit(BME280::PresUnit_Pa);

	bme.read(pres, temp, hum, tempUnit, presUnit);

	client->print("Temp: ");
	client->print(temp);
	client->print("�" + String(tempUnit == BME280::TempUnit_Celsius ? 'C' : 'F'));
	client->print("\t\tHumidity: ");
	client->print(hum);
	client->print("% RH");
	client->print("\t\tPressure: ");
	client->print(pres);
	client->println("Pa");

	delay(1000);
}

//void readCompass() {
//	int x, y, z;
//
//	// Read compass values
//	compass.read();
//
//	x = compass.getX();
//	y = compass.getY();
//	z = compass.getZ();
//
//	Serial.print("X: ");
//	Serial.print(x);
//	Serial.print("   Y: ");
//	Serial.print(y);
//	Serial.print("   Z: ");
//	Serial.println(z);
//
//	delay(300);
//}

// Specialty angular conversion functions
double degToMOA(double deg) {
	return deg * 60;
}

double degToRad(double deg) {
	return deg*M_PI / 180;
}

double moaToDeg(double moa) {
	return moa / 60;
}

double moaToRad(double moa) {
	return moa / 60 * M_PI / 180;
}

double radToDeg(double rad) {
	return rad * 180 / M_PI;
}

double radToMOA(double rad) {
	return rad * 60 * 180 / M_PI;
}

double calcFR(double temperature, double pressure, double relativeHumidity) {
	double vpw = 4e-6 * pow(temperature, 3) - 0.0004 * pow(temperature, 2) + 0.0234 * temperature - 0.2517;
	double frH = 0.995 * (pressure / (pressure - (0.3783)*(relativeHumidity) * vpw));
	return frH;
}

double calcFP(double pressure) {
	double pstd = 29.53; // in-hg
	double fp = 0;
	fp = (pressure - pstd) / (pstd);
	return fp;
}

double calcFT(double temperature, double altitude) {
	double tstd = -0.0036 * altitude + 59;
	double ft = (temperature - tstd) / (459.6 + tstd);
	return ft;
}

double calcFA(double altitude) {
	double fa = 0;
	fa = -4e-15*pow(altitude, 3) + 4e-10 * pow(altitude, 2) - 3e-5 * altitude + 1;
	return (1 / fa);
}

double atmCorrect(double dragCoefficient, double altitude, double barometer, double temperature, double relativeHumidity) {

	double fs = calcFA(altitude);
	double ft = calcFT(temperature, altitude);
	double fr = calcFR(temperature, barometer, relativeHumidity);
	double fp = calcFP(barometer);

	// Calculate the atmospheric correction factor
	double cd = (fs*(1 + ft - fp) * fr);
	return dragCoefficient * cd;
}

double retard(DragFunction dragFunction, double dragCoefficient, double velocity) {

	//	printf("DF: %d, CD: %f, V: %f,);

	double vp = velocity;
	double val = -1;
	double A = -1;
	double M = -1;
	switch (dragFunction) {
	case G1:
		if (vp > 4230) { A = 1.477404177730177e-04; M = 1.9565; }
		else if (vp> 3680) { A = 1.920339268755614e-04; M = 1.925; }
		else if (vp> 3450) { A = 2.894751026819746e-04; M = 1.875; }
		else if (vp> 3295) { A = 4.349905111115636e-04; M = 1.825; }
		else if (vp> 3130) { A = 6.520421871892662e-04; M = 1.775; }
		else if (vp> 2960) { A = 9.748073694078696e-04; M = 1.725; }
		else if (vp> 2830) { A = 1.453721560187286e-03; M = 1.675; }
		else if (vp> 2680) { A = 2.162887202930376e-03; M = 1.625; }
		else if (vp> 2460) { A = 3.209559783129881e-03; M = 1.575; }
		else if (vp> 2225) { A = 3.904368218691249e-03; M = 1.55; }
		else if (vp> 2015) { A = 3.222942271262336e-03; M = 1.575; }
		else if (vp> 1890) { A = 2.203329542297809e-03; M = 1.625; }
		else if (vp> 1810) { A = 1.511001028891904e-03; M = 1.675; }
		else if (vp> 1730) { A = 8.609957592468259e-04; M = 1.75; }
		else if (vp> 1595) { A = 4.086146797305117e-04; M = 1.85; }
		else if (vp> 1520) { A = 1.954473210037398e-04; M = 1.95; }
		else if (vp> 1420) { A = 5.431896266462351e-05; M = 2.125; }
		else if (vp> 1360) { A = 8.847742581674416e-06; M = 2.375; }
		else if (vp> 1315) { A = 1.456922328720298e-06; M = 2.625; }
		else if (vp> 1280) { A = 2.419485191895565e-07; M = 2.875; }
		else if (vp> 1220) { A = 1.657956321067612e-08; M = 3.25; }
		else if (vp> 1185) { A = 4.745469537157371e-10; M = 3.75; }
		else if (vp> 1150) { A = 1.379746590025088e-11; M = 4.25; }
		else if (vp> 1100) { A = 4.070157961147882e-13; M = 4.75; }
		else if (vp> 1060) { A = 2.938236954847331e-14; M = 5.125; }
		else if (vp> 1025) { A = 1.228597370774746e-14; M = 5.25; }
		else if (vp>  980) { A = 2.916938264100495e-14; M = 5.125; }
		else if (vp>  945) { A = 3.855099424807451e-13; M = 4.75; }
		else if (vp>  905) { A = 1.185097045689854e-11; M = 4.25; }
		else if (vp>  860) { A = 3.566129470974951e-10; M = 3.75; }
		else if (vp>  810) { A = 1.045513263966272e-08; M = 3.25; }
		else if (vp>  780) { A = 1.291159200846216e-07; M = 2.875; }
		else if (vp>  750) { A = 6.824429329105383e-07; M = 2.625; }
		else if (vp>  700) { A = 3.569169672385163e-06; M = 2.375; }
		else if (vp>  640) { A = 1.839015095899579e-05; M = 2.125; }
		else if (vp>  600) { A = 5.71117468873424e-05; M = 1.950; }
		else if (vp>  550) { A = 9.226557091973427e-05; M = 1.875; }
		else if (vp>  250) { A = 9.337991957131389e-05; M = 1.875; }
		else if (vp>  100) { A = 7.225247327590413e-05; M = 1.925; }
		else if (vp>   65) { A = 5.792684957074546e-05; M = 1.975; }
		else if (vp>    0) { A = 5.206214107320588e-05; M = 2.000; }
		break;

	case G2:
		if (vp> 1674) { A = .0079470052136733;  M = 1.36999902851493; }
		else if (vp> 1172) { A = 1.00419763721974e-03;  M = 1.65392237010294; }
		else if (vp> 1060) { A = 7.15571228255369e-23;  M = 7.91913562392361; }
		else if (vp>  949) { A = 1.39589807205091e-10;  M = 3.81439537623717; }
		else if (vp>  670) { A = 2.34364342818625e-04;  M = 1.71869536324748; }
		else if (vp>  335) { A = 1.77962438921838e-04;  M = 1.76877550388679; }
		else if (vp>    0) { A = 5.18033561289704e-05;  M = 1.98160270524632; }
		break;

	case G5:
		if (vp> 1730) { A = 7.24854775171929e-03; M = 1.41538574492812; }
		else if (vp> 1228) { A = 3.50563361516117e-05; M = 2.13077307854948; }
		else if (vp> 1116) { A = 1.84029481181151e-13; M = 4.81927320350395; }
		else if (vp> 1004) { A = 1.34713064017409e-22; M = 7.8100555281422; }
		else if (vp>  837) { A = 1.03965974081168e-07; M = 2.84204791809926; }
		else if (vp>  335) { A = 1.09301593869823e-04; M = 1.81096361579504; }
		else if (vp>    0) { A = 3.51963178524273e-05; M = 2.00477856801111; }
		break;

	case G6:
		if (vp> 3236) { A = 0.0455384883480781; M = 1.15997674041274; }
		else if (vp> 2065) { A = 7.167261849653769e-02; M = 1.10704436538885; }
		else if (vp> 1311) { A = 1.66676386084348e-03; M = 1.60085100195952; }
		else if (vp> 1144) { A = 1.01482730119215e-07; M = 2.9569674731838; }
		else if (vp> 1004) { A = 4.31542773103552e-18; M = 6.34106317069757; }
		else if (vp>  670) { A = 2.04835650496866e-05; M = 2.11688446325998; }
		else if (vp>    0) { A = 7.50912466084823e-05; M = 1.92031057847052; }
		break;

	case G7:
		if (vp> 4200) { A = 1.29081656775919e-09; M = 3.24121295355962; }
		else if (vp> 3000) { A = 0.0171422231434847; M = 1.27907168025204; }
		else if (vp> 1470) { A = 2.33355948302505e-03; M = 1.52693913274526; }
		else if (vp> 1260) { A = 7.97592111627665e-04; M = 1.67688974440324; }
		else if (vp> 1110) { A = 5.71086414289273e-12; M = 4.3212826264889; }
		else if (vp>  960) { A = 3.02865108244904e-17; M = 5.99074203776707; }
		else if (vp>  670) { A = 7.52285155782535e-06; M = 2.1738019851075; }
		else if (vp>  540) { A = 1.31766281225189e-05; M = 2.08774690257991; }
		else if (vp>    0) { A = 1.34504843776525e-05; M = 2.08702306738884; }
		break;

	case G8:
		if (vp> 3571) { A = .0112263766252305; M = 1.33207346655961; }
		else if (vp> 1841) { A = .0167252613732636; M = 1.28662041261785; }
		else if (vp> 1120) { A = 2.20172456619625e-03; M = 1.55636358091189; }
		else if (vp> 1088) { A = 2.0538037167098e-16; M = 5.80410776994789; }
		else if (vp>  976) { A = 5.92182174254121e-12; M = 4.29275576134191; }
		else if (vp>    0) { A = 4.3917343795117e-05; M = 1.99978116283334; }
		break;

	default:
		break;
	}

	if (A != -1 && M != -1 && vp>0 && vp<10000) {
		val = A*pow(vp, M) / dragCoefficient;
		return val;
	}
	else return -1;
}

double getRange(double* sln, int yardage) {
	double size = sln[__BCOMP_MAXRANGE__ * 10 + 1];
	if (yardage<size) {
		return sln[10 * yardage];
	}
	else return 0;
}

double getPath(double* sln, int yardage) {
	double size = sln[__BCOMP_MAXRANGE__ * 10 + 1];
	if (yardage<size) {
		return sln[10 * yardage + 1];
	}
	else return 0;
}

double getMOA(double* sln, int yardage) {
	double size = sln[__BCOMP_MAXRANGE__ * 10 + 1];
	if (yardage<size) {
		return sln[10 * yardage + 2];
	}
	else return 0;
}

double getTime(double* sln, int yardage) {
	double size = sln[__BCOMP_MAXRANGE__ * 10 + 1];
	if (yardage<size) {
		return sln[10 * yardage + 3];
	}
	else return 0;
}

double getWindage(double* sln, int yardage) {
	double size = sln[__BCOMP_MAXRANGE__ * 10 + 1];
	if (yardage<size) {
		return sln[10 * yardage + 4];
	}
	else return 0;
}

double getWindageMOA(double* sln, int yardage) {
	double size = sln[__BCOMP_MAXRANGE__ * 10 + 1];
	if (yardage<size) {
		return sln[10 * yardage + 5];
	}
	else return 0;
}

double getVelocity(double* sln, int yardage) {
	double size = sln[__BCOMP_MAXRANGE__ * 10 + 1];
	if (yardage<size) {
		return sln[10 * yardage + 6];
	}
	else return 0;
}

double getVx(double* sln, int yardage) {
	double size = sln[__BCOMP_MAXRANGE__ * 10 + 1];
	if (yardage<size) {
		return sln[10 * yardage + 7];
	}
	else return 0;
}

double getVy(double* sln, int yardage) {
	double size = sln[__BCOMP_MAXRANGE__ * 10 + 1];
	if (yardage<size) {
		return sln[10 * yardage + 8];
	}
	else return 0;
}

double zeroAngle(DragFunction dragFunction, double dragCoefficient, double vi, double sightHeight, double zeroRange, double yIntercept) {

	// Numerical Integration variables
	double t = 0;
	double dt = 1 / vi; // The solution accuracy generally doesn't suffer if its within a foot for each second of time.
	double y = -sightHeight / 12;
	double x = 0;
	double da; // The change in the bore angle used to iterate in on the correct zero angle.

			   // State variables for each integration loop.
	double v = 0, vx = 0, vy = 0; // velocity
	double vx1 = 0, vy1 = 0; // Last frame's velocity, used for computing average velocity.
	double dv = 0, dvx = 0, dvy = 0; // acceleration
	double Gx = 0, Gy = 0; // Gravitational acceleration

	double angle = 0; // The actual angle of the bore.

	int quit = 0; // We know it's time to quit our successive approximation loop when this is 1.

				  // Start with a very coarse angular change, to quickly solve even large launch angle problems.
	da = degToRad(14);

	// The general idea here is to start at 0 degrees elevation, and increase the elevation by 14 degrees
	// until we are above the correct elevation.  Then reduce the angular change by half, and begin reducing
	// the angle.  Once we are again below the correct angle, reduce the angular change by half again, and go
	// back up.  This allows for a fast successive approximation of the correct elevation, usually within less
	// than 20 iterations.
	for (angle = 0; quit == 0; angle = angle + da) {
		vy = vi * sin(angle);
		vx = vi * cos(angle);
		Gx = GRAVITY * sin(angle);
		Gy = GRAVITY * cos(angle);

		for (t = 0, x = 0, y = -sightHeight / 12; x <= zeroRange * 3; t = t + dt) {
			vy1 = vy;
			vx1 = vx;
			v = pow((pow(vx, 2) + pow(vy, 2)), 0.5);
			dt = 1 / v;

			dv = retard(dragFunction, dragCoefficient, v);
			dvy = -dv*vy / v*dt;
			dvx = -dv*vx / v*dt;

			vx = vx + dvx;
			vy = vy + dvy;
			vy = vy + dt*Gy;
			vx = vx + dt*Gx;

			x = x + dt*(vx + vx1) / 2;
			y = y + dt*(vy + vy1) / 2;
			// Break early to save CPU time if we won't find a solution.
			if (vy<0 && y<yIntercept) {
				break;
			}
			if (vy > 3 * vx) {
				break;
			}
		}

		if (y>yIntercept && da>0) {
			da = -da / 2;
		}

		if (y<yIntercept && da<0) {
			da = -da / 2;
		}

		if (fabs(da) < moaToRad(0.01)) quit = 1; // If our accuracy is sufficient, we can stop approximating.
		if (angle > degToRad(45)) quit = 1; // If we exceed the 45 degree launch angle, then the projectile just won't get there, so we stop trying.
	}
	return radToDeg(angle); // Convert to degrees for return value.
}

int solveAll(DragFunction dragFunction, double dragCoefficient, double vi, double sightHeight, \
	double shootingAngle, double zAngle, double windSpeed, double windAngle, double range) {

	double t = 0;
	double dt = 0.5 / vi;
	double v = 0;
	double vx = 0, vx1 = 0, vy = 0, vy1 = 0;
	double dv = 0, dvx = 0, dvy = 0;
	double x = 0, y = 0;

	double headwind = headWind(windSpeed, windAngle);
	double crosswind = crossWind(windSpeed, windAngle);

	double gy = GRAVITY * cos(degToRad((shootingAngle + zAngle)));
	double gx = GRAVITY * sin(degToRad((shootingAngle + zAngle)));

	vx = vi * cos(degToRad(zAngle));
	vy = vi * sin(degToRad(zAngle));

	y = -sightHeight / 12;

	int n = 0;
	for (t = 0;; t = t + dt) {

		vx1 = vx, vy1 = vy;
		v = pow(pow(vx, 2) + pow(vy, 2), 0.5);
		dt = 0.5 / v;

		// Compute acceleration using the drag function retardation	
		dv = retard(dragFunction, dragCoefficient, v + headwind);
		dvx = -(vx / v) * dv;
		dvy = -(vy / v) * dv;

		// Compute velocity, including the resolved gravity vectors.	
		vx = vx + dt * dvx + dt * gx;
		vy = vy + dt * dvy + dt * gy;

		if (x / 3 >= range) {
			range = x / 3;								// Range in yds
			path = y * 12;								// Path in inches
			elevationMOA = -radToMOA(atan(y / x));		// Correction in MOA
			flightTime = t + dt;						// Time in s
			windage = windageDelta(crosswind, vi, x, t + dt);// Windage in inches
			windageMOA = radToMOA(atan(windage));		// Windage in MOA
			velocityCombined = v;						// Velocity (combined)
			velocityX = vx;								// Velocity (x)
			velocityY = vy;								// Velocity (y)
			break;
		}

		// Compute position based on average velocity.
		x = x + dt*(vx + vx1) / 2;
		y = y + dt*(vy + vy1) / 2;

		if (fabs(vy)>fabs(3 * vx)) break;
		if (n >= __BCOMP_MAXRANGE__ + 1) break;
	}
	return n;
}

double windageDelta(double windSpeed, double vi, double xx, double t) {
	double vw = windSpeed * 17.60; // Convert to inches per second.
	return (vw * (t - xx / vi));
}

// Headwind is positive at WindAngle=0
double headWind(double windSpeed, double windAngle) {
	double windageInRads = degToRad(windAngle);
	return (cos(windageInRads)*windSpeed);
}

// Positive is from Shooter's Right to Left (Wind from 90 degree)
double crossWind(double windSpeed, double windAngle) {
	double windageInRads = degToRad(windAngle);
	return (sin(windageInRads) * windSpeed);
}
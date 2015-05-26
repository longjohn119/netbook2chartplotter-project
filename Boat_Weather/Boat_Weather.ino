/* A program to read temperature, humidity, dew point, heat index and barometric pressure
   and send data to computer via USB serial
*/

#include <DHT.h>
#include <SFE_BMP180.h>
#include <Wire.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define ALTITUDE 200   // Altitude of Home, Credit Island 188 meters, Lost Grove Lake 216 meters

DHT dht(DHTPIN, DHTTYPE);
float tF;
float dP;
float dPF;

SFE_BMP180 pressure;


void setup() {
  Serial.begin(9600);
  dht.begin();
  pressure.begin();
  //if (pressure.begin())
  //Serial.println("BMP180 init success");
        
  //else
  //{
   // Serial.println("BMP180 init fail\n\n");
   // while (1);
  //}
}

void loop() {
  
   // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT");
  } 
  else {
    //Serial.println("");
    Serial.println("");
    Serial.print("Humidity:    ");
    Serial.print(h);
    Serial.println(" % ");
    Serial.println("");
    Serial.print("Temperature: ");
    // Serial.print(t);
    // Serial.print(" *C ");
    tF=((t*9)/5)+32;
    Serial.print(tF);
    Serial.println(" *F ");
    Serial.println("");
    Serial.print("Dew Point:   ");
    // Serial.print(dewPointFast(t, h));
    // Serial.print(" *C ");
    dP=(dewPointFast(t, h));
    dPF=((dP*9)/5)+32;
    Serial.print(dPF);
    Serial.println(" *F");
    Serial.println("");
    Serial.print("Heat Index:  ");
    // Heat Index calculation only valid if temperature is 80F or above 
    // and the humidity is 40% or above - Print ----- if not true
    if (tF < 80 || h < 40) {
      Serial.print("-----");
    } 
    else {
      Serial.print(heatIndex(tF,h));
    }
    Serial.println(" *F");
    Serial.println("");
    
    char status;
  double T,P,p0,a;

   status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
      if (status != 0)
    {
  status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          
            p0 = pressure.sealevel(P,ALTITUDE); // we're at 1655 meters (Boulder, CO)
          Serial.print("Relative Pressure: ");
          Serial.print(p0,2);
          Serial.print(" mb,    ");
          Serial.print(p0*0.0295333727,2);
          Serial.println(" inHg");
        }
         else Serial.println("error retrieving pressure measurement\n");
         }
      else Serial.println("error starting pressure measurement\n"); 
      }
         else Serial.println("error retrieving temperature measurement\n"); 
     }
      else Serial.println("error starting temperature measurement\n");    
    }
      
    Serial.println("");
    //Serial.println("");
    delay(60000);


  }
  


// delta max = 0.6544 wrt dewPoint()
// 6.9 x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity)
{
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity*0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}

double heatIndex(double tempF, double humidity)
{
  double c1 = -42.38, c2 = 2.049, c3 = 10.14, c4 = -0.2248, c5= -6.838e-3, c6=-5.482e-2, c7=1.228e-3, c8=8.528e-4, c9=-1.99e-6  ;
  double T = tempF;
  double R = humidity;

  double A = (( c5 * T) + c2) * T + c1;
  double B = ((c7 * T) + c4) * T + c3;
  double C = ((c9 * T) + c8) * T + c6;

  double rv = (C * R + B) * R + A;
  return rv;
}

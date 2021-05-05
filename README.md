# BallisticComputer002
A ballistic computer designed to run on an ESP32 device.

Shooting over long distances, say greater than 1000m requires the shooter to take into account a number of environmental and other 
conditions.  The shooter needs to know the temperature, atmospheric pressure, relative humidity, wind direction and speed.  Along with these the shooter needs to make allowance for range, Coriolis forces and the vertical angle between shooter and target.  And last but not least the projectile weight, ballistic coefficient, the distance between the scope and the center of the barrel and muzzle velocity.

My aim for this project is an attempt to design and build a ballistic computer.  It must capable of capturing user inputs.  i.e. bullet weight, ballistic coefficient, range to target and muzzle velocity etc. It must make all the environmental measurements and generate a firing solution.  The solution could be delivered as an MOA correction for elevation and windage or graphically as an aiming point on a mil dot or equivalent reticule.

To process all this information is I believe well beyond the capability of say an Aruino Uno.  There is a device though that just might be
up to the task.  It is the Expressif ESP32.

My current thoughts on a bill of materials are:<br>
1 x ESP32                 - Microcontroller<br>
1 x BME280                - Pressure, temperature and relative humidity        
1 x UBLOX NEO-6M          - Latitude and altitude<br>
1 x ITEAD Nextion 5.0"    - Touch Display<br>
1 x MPU-6050              - 6 DOF gyro and compass<br>
1 x WeatherFlow WINDMeter - Anemometer<br>
1 x Buck converter        - To generate 5V for the Nextion<br>
1 x SD data logger        - To record entered data

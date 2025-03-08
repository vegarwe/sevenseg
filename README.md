Simple seven segment display test

* For NodeMCU the pins used are D21 for SDA and D22 for SCL, see Wire.cpp
* For Feather the pins used are D23 for SDA and D22 for SCL

To see which, just do `Serial.printf("TwoWire setup: SDA %u SCL %u\n", SDA, SCL);`

TODO: Different brightness based on time of day

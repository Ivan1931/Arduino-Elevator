/**SUMMARY**
    * The elevator program waits until a button is selected. When a button is selected the elevator will move towards its associated floor
    
    * The elevator will visit all floors that have been called in its current direction before changing direction
    
    * If more than one button is called in the same direction, the elevator will visit either the highest or the lowest
      of the buttons that have been selected depending on its current direction first before changing direction and executing the current task.
      For EXAMPLE, if the top button is called, and the second story going down button is called, the elevator will skip the second story and proceed to the top floor
      and then stop at the second story on its way down.
      
    * The elevevator will always stop at a story where a corresponding interiorButton (Button inside the elevator) is selected
    
    * The elevator will stop at the top and bottom stories
*/
const int exteriorButtonPins[] = {10,11,12,13};
const int interiorButtonPins[] = {6,7,8};

//Set to true when the associated interior or exterior buttons are selected
const int NUM_INTERIOR = 3;
const int NUM_EXTERIOR = 4;
boolean interiorSelected[] = {false,false,false};
boolean exteriorSelected[] = {false,false,false,false};

const int NUM_LEDS = 6;
const int outputLEDPins[] = {A0, A1, A2, A3, A4, A5};

const int engineForwardPin = 5;
const int engineBackwardPin = 4;

const int checkFloorPin = 3;

int currentFloor = 0;

const int MAX_FLOOR_INDEX = 2;
const int MIN_FLOOR_INDEX = 0;
const int NUM_FLOORS = 3;

boolean currentDirection = true; //Stores the current direction the elevator is travelling. true is up. false is down
boolean halt = true; //True when elevator is stationary

boolean floorDetectionLock = true;
unsigned long timeAtUnlocked;
const unsigned long MAX_BETWEEN_FLOOR_SENSED = 250;

const float FLOOR_HEIGHT = 116.6; //mm
void setup () {
    Serial.begin (9600);
    setupLEDPins ();
    setupInteriorButtons ();
    setupExteriorButtons ();
    pinMode(engineForwardPin, OUTPUT);
    pinMode(engineBackwardPin, OUTPUT);
}

void loop () {
   /* if (DEBUG) {
        currentFloor = Serial.read ();
        DEBUG = false;
    }*/
    if (!halt) {
        if (hasReachedFloor()) {
                floorReachedEvent(); 
                if (shouldStopAtFloor (currentFloor, currentDirection)) {
                    Serial.print("Stopped at floor : " );
                    Serial.println (currentFloor);
                    stopElevator ();
                    delay (2000);
                    markFloorAsVisited(currentFloor);  
                    Serial.println ("*******************");
                } 
                executeTask();
        }
    }
    if (buttonPressDetected()) 
            executeTask ();
    updateLEDs ();    
}

/**
    Gets the elevator to start moving towards the next floor it should visit
*/
void executeTask(){
    if (taskInDirection(currentDirection))
          startElevator ();
    else if (taskInDirection (!currentDirection)) {
          changeDirection();
          startElevator();
   }
}

/**
  Returns true if there is still a floor to visit in direction specified
*/
boolean taskInDirection(boolean pcurrentDirection) {
    for (int i = currentFloor; pcurrentDirection ? i < 3 : i >= 0; i = pcurrentDirection ? i + 1 : i - 1) //Muhahahahahahahahaha
    {
        if (shouldStopAtFloor(i,pcurrentDirection))
          return true;
    }
    return false;
}

void setupExteriorButtons () {
    for (int i = 0; i < NUM_EXTERIOR; i++)
        pinMode (exteriorButtonPins[i], INPUT);
}

void setupInteriorButtons () {
    for (int i = 0 ; i < NUM_INTERIOR; i++)
        pinMode (interiorButtonPins[i], INPUT);
}

void setupLEDPins () {
    for (int i = 0 ; i < NUM_LEDS; i++) 
        pinMode (outputLEDPins[i], OUTPUT);
}

//Returns true if a button press is detected
boolean buttonPressDetected () {
    
    return (readExteriorButtonStates ()
           || readInteriorButtonStates ());
    
}

boolean readExteriorButtonStates () {
    int count = 0;
    for (int i = 0 ; i < 4; i++) {
            if (count != getFloorForExteriorButton(i))
                  count++;
            if (digitalRead (exteriorButtonPins[i]) == HIGH && count != currentFloor) {
                exteriorSelected[i] = true;
                Serial.print ("Read exterior button ");
                Serial.println (i);
                return true;
            }
            
    }
    return false;
}

boolean hasReachedFloor () {
    if (digitalRead (checkFloorPin) && !floorDetectionLock && (millis() - timeAtUnlocked > MAX_BETWEEN_FLOOR_SENSED)) {
        Serial.println("Detected reached floor");
        //delay(100);
        return true;
    }
    return false;
}

/**
    Digital reads all of the buttons and updates the interiorSelected flags if a button is pressed
*/
boolean readInteriorButtonStates  () {
    for (int i = 0 ; i < 3; i++)  {
        if (i != currentFloor) {
            if (digitalRead(interiorButtonPins[i]) && !interiorSelected[i]) {
                interiorSelected[i] = true;
                Serial.print ("Read interior button ");
                Serial.println (i);
                return true;
            }
        }
    }
    return false;
}

/**
  Returns true if an interior button selection for the floor at index specified was made or an 
  exterior button that is in the same direction as the current floors direction has been selected.
*/
boolean shouldStopAtFloor (int floorIndex, boolean pcurrentDirection) {
     switch (floorIndex) {
         case 0: 
             return shouldStopAtFloorZero(pcurrentDirection);
         case 1: 
             return shouldStopAtFloorOne(pcurrentDirection);
         default:
              return shouldStopAtFloorTwo (pcurrentDirection);
     }
}

boolean shouldStopAtFloorZero (boolean pcurrentDirection) {
    return (!pcurrentDirection && exteriorSelected[0]) ||interiorSelected[0];    
}

boolean shouldStopAtFloorOne (boolean pcurrentDirection) {
    return (pcurrentDirection && exteriorSelected[2])
              || (!pcurrentDirection && exteriorSelected[1])
              || (
                     (exteriorSelected[1] || exteriorSelected[2]) 
                           && (pcurrentDirection ? !shouldStopAtFloorTwo(pcurrentDirection) : !shouldStopAtFloorZero(pcurrentDirection))
                 )
              || interiorSelected[1];
}

boolean shouldStopAtFloorTwo (boolean pcurrentDirection) {
    return (pcurrentDirection && exteriorSelected[3]) || interiorSelected[2];
}

/**
    Increments or decrements floorReached variable depending on direction of the elevator
    Ensures that the floor is marked as visited by deactivating its floor visited commands
    Returns false if the elevator is at the top or bottomFloor
*/
boolean floorReachedEvent() {
    boolean succesfull = true;
    floorDetectionLock = true;
    if (currentDirection) {
          if (currentFloor < MAX_FLOOR_INDEX) 
              currentFloor++;
          else {
              Serial.println("Reached top floor!");
              succesfull = false;
          }
      } else {
          if (currentFloor > MIN_FLOOR_INDEX)
              currentFloor--;
          else {
              Serial.println("Reached bottom floor");
              succesfull = false;
          }
    }  
    return succesfull;
}

void markFloorAsVisited (int floorIndex) {
    interiorSelected[floorIndex] = false;
    switch (floorIndex) {
        case 0:
          exteriorSelected[0] = false;
        break;
        case 1:
           if (currentDirection)
               exteriorSelected[2] = false;
           else
               exteriorSelected[1] = false;
            if (!shouldStopAtFloorTwo(currentDirection) && !shouldStopAtFloorZero(currentDirection))
            {
                 exteriorSelected[2] = false;
                 exteriorSelected[1] = false;
            }
        break;
        case 2:
           exteriorSelected[3] = false;
        break;
    }
}

void changeDirection () {
      Serial.print("Changed direction to: " );
      Serial.println (!currentDirection);
      currentDirection = !currentDirection;
      digitalWrite (engineForwardPin, !currentDirection);
      digitalWrite (engineBackwardPin, currentDirection);
}

void stopElevator () {
      Serial.print("Did stop at floor: " ) ;
      Serial.println (currentFloor);
      Serial.print ("Time between floors: ");
      //Serial.println ((float)FLOOR_HEIGHT/ (float)(timeAtUnlocked - millis()));
      digitalWrite (engineForwardPin,LOW);
      digitalWrite (engineBackwardPin, LOW);
      halt = true;
}

void startElevator () {
      Serial.print("Started from floor: ");
      Serial.println (currentFloor);
      digitalWrite (engineForwardPin, currentDirection);
      digitalWrite (engineBackwardPin, !currentDirection);
      halt = false;
      floorDetectionLock = false;
      timeAtUnlocked = millis();
}

void updateLEDs () {
     for (int i = 0; i < NUM_FLOORS; i++) {
         if (shouldStopAtFloor(i, currentDirection))
               digitalWrite (outputLEDPins[i + NUM_FLOORS], HIGH);
         else 
               digitalWrite (outputLEDPins [i + NUM_FLOORS],LOW);
         if (i == currentFloor) 
               digitalWrite (outputLEDPins[i], HIGH);
          else 
               digitalWrite (outputLEDPins[i], LOW);
             
     }
}

void printExteriorButtonStates () {
      for (int i = 0 ; i < NUM_EXTERIOR; i++) {
            Serial.print ("Exterior Button : ");
            Serial.print (i);
            Serial.println (exteriorSelected[i] ? " TRUE " : " FALSE ");
      }
}

void printInteriorButtonStates () {
    for (int i = 0 ; i < NUM_INTERIOR; i++) {
            Serial.print ("Interior Button : ");
            Serial.print (i);
            Serial.println (interiorSelected[i] ? " TRUE " : " FALSE ");
      }
}

int getFloorForExteriorButton (int buttonIndex) {
      switch (buttonIndex) {
           case 0: return 0;
           case 3: return 2;
           default: return 1;
      }
}

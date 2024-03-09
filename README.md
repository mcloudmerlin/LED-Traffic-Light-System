Introduction

The goal of this project is to implement a traffic simulation using hardware and software. The hardware components consist of LED’s, three shift registers, a potentiometer, resistors, and wires. The software is written in C and uses FreeRTOS, a real time operating system for microcontrollers. The features used for the project include tasks, queues, and software timers. 

The project simulates a one way vehicle traffic intersection (no cross street). The car positions are represented by a horizontal line of green LED’s. When the LED is on, a car is present at that position, and when an LED is off, no car is currently at the position. Splitting the car LED’s into two halves is three other LED’s (green, yellow, and red) that represent traffic lights. The cars proceed through the intersection when the light is green or yellow, but must stop at the intersection when the light is red. This means that while the traffic light is red, the cars before the intersection bunch up filling any empty positions. The cars after the intersection aren’t affected by the traffic light. 

The rate at which traffic is generated is controlled by a potentiometer. The potentiometer value is also proportional to the duration of the traffic lights. This project relies heavily on the flow of data between the hardware and software components. 
Design Solution 
Our design involved breaking the problem down into smaller subtasks. These could be categorized as circuit design, GPIO initialization, ADC initialization, traffic flow adjustment, traffic generation, LED display, traffic light timers.

Our design process began with creating an early sketch of the overall data flow and description of how the four tasks and queues worked together. This is shown in image 1 above. Overall, the sketch is a good representation of the final design. The most significant change that we made was the removal of the “traffic light state task”. We instead used software timer callback functions to control the switching and duration of the traffic lights and the task became unnecessary. 

Circuit design

Our circuit involved three serialized 74HC164 shift registers. These are serialized by synchronizing the clock and reset pins of all three shift registers, as well as connecting the H pin to the next shift registers data pin. Each output pin from the shift registers was connected to an LED representing a car. Our traffic lights were controlled directly by GPIOB ports. The potentiometer was set up in a standard way, drawing 5 volts and sending its output to GPIOA.

GPIO initialization

To initialize our GPIO, we first enabled all of the GPIO clocks. We then configured GPIOC for the shift registers, and GPIOB for the traffic light control. These were configured exactly as described in the provided slides. We also made sure to set the shift register reset bit to 0 at this point.
ADC initialization
To initialize our ADC, we first enabled the ADC clock. We then configured the ADC exactly as described in the provided slides. We then enabled the ADC, and set it to channel 1.
Traffic flow adjustment
Our traffic flow adjustment task was responsible for detecting a change in the value coming from the ADC, and if a change was detected, sending this new value to the traffic light timers via a queue.

Traffic generation

Our traffic generator task was responsible for generating a random number between 0 and 5000. If the ADC value was less than this random number, it would send a 1 to the LED system display task via a queue. If the ADC value was greater than the random number, it would send a 0 to the LED system display task via queue.
LED system display

The system display algorithm controls the LED’s representing the car position values. A car position LED can be on or off, representing a car at that position or no car at that position. A flow chart describing the algorithm is seen in image 4.

The LED system display task first creates an array of size 19 to represent the cars in traffic and initializes it to be filled with 0s. The first value of the array is set to the value received from the traffic generation task. If the light is green, the array is looped through backwards, setting every value in the array to be equal to the value of the index directly below it. This simulated the movement of traffic. If the light is yellow or red, the array entries after the traffic light are looped through backwards and updated to simulate normal traffic movement, but the entries before the traffic lights are treated differently. They are looped through backwards, but the values are not updated to simulate traffic movement unless a value of 0 is encountered, at which point a flag is set to 1, and all values encountered after this are updated to simulate traffic movement. This simulates traffic coming to a stop at the red light, and then stacking up close together.
Traffic light timers
We created three timers, one to represent each traffic light color. We then created three callback functions. At the start of our program the light was set to green, and our green light callback function was called. 
Once the set time had elapsed, the green light callback function was executed. This function set the light to yellow, sent this value to other functions via queue, and then called the amber light callback function.
Once the set time had elapsed, the yellow light callback function was executed. This function set the light to red, and sent this value to other functions via queue. The period for the red light timer was then calculated based on the ADC value (received from a queue). The period was calculated to be inversely proportional to the ADC value. The red light callback function was then called.
Once the set time had elapsed, the red light callback function was executed. This function set the light to green, and sent this value to other functions via queue. The period for the green light timer was then calculated based on the ADC value (received from a queue). The period was calculated to be directly proportional to the ADC value. The green light callback function was then called.

Discussion

In this project, we attempted to use the specifications given in the lab manual and lab slides as closely as possible. This way, we could focus on ensuring our software was well planned and developed, rather than worrying about if problems were coming from venturing away from the lab manual specifications.

Our implementation was smooth for the most part. Early on, we ran into some trouble getting started with the circuitry and getting LEDs to turn on. This turned out to be mostly due to incorrect initialization functions. Once we got these sorted out, we improved our project steadily throughout the implementation project. 

Before writing any software, we planned out how our code would be split into tasks and functions. This made the development process much more efficient, as there were very few instances of creating esoteric and overly complex code. We also made use of the oscilloscope for this project. This was very useful at certain points, as it was at times not clear our software was failing to pick up on an input signal, or if the signal was failing to be sent.
Limitations and Possible Improvements
Looking back on this project, we could have improved our breadboard circuitry techniques. We got caught up in the development cycle and failed to plan well at the start, which created a confusing mess of wires. This was improved later on, but if we had planned out our circuitry before starting the development process it would have saved a lot of headaches and debugging.

Summary

The purpose of this project was to get hands-on experience with real time systems by creating a simulated intersection using LEDs to represent cars and a potentiometer to control the flow of traffic. The project was somewhat difficult to start, as we had limited experience with some of the hardware modules. But due to proper design planning at the beginning, once we got started our project moved along steadily. Most of our bugs originated at the intersection of software and hardware. It was difficult to know if the hardware was failing or if the software was written incorrectly. However, the oscilloscope was very useful when diagnosing these issues.
Despite some setbacks early on, we were able to implement a fully functional project with time to spare before the deadline, which allowed us to spend some time cleaning up our circuitry, as well as our code.

Overall, a very satisfying project once all of the hardware and software components were working well together.

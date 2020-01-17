#define INPUT_PIN 7
#define OUTPUT_PIN 13
#define TASK_LENGTH_MS 100
#define RECEIVE_SUBSAMPLING_RATIO 10
#define MESSAGE_BIT_LENGTH 8

const unsigned int decisiveBitPosition = floor((RECEIVE_SUBSAMPLING_RATIO * 80) / 100);
char payloadToSend = 'A'; // * d42	0b 0010 1010  --> 0101 0100

String readBuffer;

bool signalHistory[RECEIVE_SUBSAMPLING_RATIO];

void handleTx();
void handleRx();
void handleLogs();

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(OUTPUT_PIN, OUTPUT);
  pinMode(INPUT_PIN, INPUT);

  readBuffer = String();

  for (int i = 0; i < RECEIVE_SUBSAMPLING_RATIO; i++)
    signalHistory[i] = 0;
}

void loop()
{
  static unsigned long startTime = millis();
  static unsigned long endTime = millis();
  static float CPULoad = 0;

  const unsigned long currentTime = millis();

  if (currentTime - endTime > TASK_LENGTH_MS)
  {
    handleTx();
    handleRx();
    handleLogs();

    startTime = currentTime;
    endTime = millis();
    CPULoad = (endTime - startTime) / TASK_LENGTH_MS;
  }
}

void sendEndBit(unsigned int *iterationsToSkip, bool *flagEndOfMessage)
{
  digitalWrite(OUTPUT_PIN, HIGH);

  *iterationsToSkip = 5;
  *flagEndOfMessage = false;

  payloadToSend++;

  if (payloadToSend > 140)
    payloadToSend = 40;

  Serial.print("\nSent end bit\n");
  Serial.print("Next is: ");
  Serial.println(payloadToSend);
}

void sendStartBit(bool *isStartBitSent)
{
  digitalWrite(OUTPUT_PIN, HIGH);
  *isStartBitSent = true;

  Serial.print("Sent start bit\n");
}

void sendNextBit(bool *isStartBitSent, unsigned int *bitIndex, bool *flagEndOfMessage)
{
  const bool signalToSend = (payloadToSend >> *bitIndex) & 1; // what bit to send

  *bitIndex = *bitIndex + 1;

  digitalWrite(OUTPUT_PIN, signalToSend);

  if (*bitIndex == MESSAGE_BIT_LENGTH)
  {
    *bitIndex = 0;

    *flagEndOfMessage = true; // if we finished transferring the bits from the character flag it so we send start bit again when we send the next message
    *isStartBitSent = false;
  }

  if (signalToSend)
    Serial.print("Sending 1\n");
  else
    Serial.print("Sending 0\n");

  // if (*flagEndOfMessage)
  // Serial.print("****\n");
}

void handleTx()
{
  static unsigned int callIteration = 0;
  static unsigned int bitIndex = 0;
  static unsigned int iterationsToSkip = 0;

  static bool isStartBitSent = false;
  static bool flagEndOfMessage = false;

  callIteration++;

  if (callIteration < RECEIVE_SUBSAMPLING_RATIO) // send next signal when the time has come - - - - - now 0 - - - - - now 1 - - - - - where - is a skipped iteration but rx is happening
    return;

  callIteration = 0;

  if (flagEndOfMessage)
    return sendEndBit(&iterationsToSkip, &flagEndOfMessage);

  /* 
    if (iterationsToSkip > 0)
    // {
    //   digitalWrite(OUTPUT_PIN, LOW);
    //   iterationsToSkip--;

    //   // Serial.print(iterationsToSkip);
    //   // Serial.print(" Skipped sending\n");

    //   return;
    // 
  */

  if (bitIndex == 0 && isStartBitSent == false) // check if we must send start bit
    return sendStartBit(&isStartBitSent);

  sendNextBit(&isStartBitSent, &bitIndex, &flagEndOfMessage);
}

void awaitEndOfMessage(bool decisiveBit, char *outputByte, bool *isStartBitReceived, unsigned int *numberOfBitsRead, bool *awaitEndBit, bool *isSamplingStarted)
{
  if (!decisiveBit)
    return;

  Serial.print("\nReceived END bit\nMessage read:");
  Serial.println(*outputByte, BIN);
  Serial.println(*outputByte);

  *outputByte = 0;
  *isStartBitReceived = 0;
  *numberOfBitsRead = 0;
  *awaitEndBit = false;
  *isSamplingStarted = false;
}

void awaitStartOfMessage(bool decisiveBit, bool *isStartBitReceived)
{
  if (decisiveBit)
  {
    *isStartBitReceived = true;
    Serial.print("Received start bit\n");
  }
}

void readBit(char *outputByte, bool decisiveBit, unsigned int *numberOfBitsRead, bool *awaitEndBit)
{
  static unsigned int bitIndex = 0;

  *outputByte = (*outputByte) | (char)(decisiveBit << bitIndex++);

  Serial.print(decisiveBit);
  Serial.print(" -> rec -> now: ");

  char temp = *outputByte;
  Serial.println(temp, BIN);

  *numberOfBitsRead = *numberOfBitsRead + 1;

  if (*numberOfBitsRead == MESSAGE_BIT_LENGTH)
  {
    *awaitEndBit = true;
    bitIndex = 0;

    readBuffer += *outputByte;
    Serial.println(readBuffer);
  }
}

void handleRx()
{
  static unsigned int numberOfBitsRead = 0;  // counts the number of bits read
  static unsigned int samplingIteration = 0; // counts function calls

  static char outputByte = 0;

  static bool isStartBitReceived = false;
  static bool isSamplingStarted = false;
  static bool awaitEndBit = false;

  const bool receivedSignal = digitalRead(INPUT_PIN); // TODO make this IN

  if (!isSamplingStarted)
    if (receivedSignal)
    {
      isSamplingStarted = true;
      samplingIteration = 0;
    }
    else
      return;

  samplingIteration++;

  signalHistory[samplingIteration - 1] = receivedSignal;

  // Serial.print(samplingIteration);
  // Serial.print(" > ");

  // for (int i = 0; i < RECEIVE_SUBSAMPLING_RATIO; i++)
  // {
  //   Serial.print(signalHistory[i]);
  //   Serial.print(", ");
  // }
  // Serial.print("\n");

  if (samplingIteration < RECEIVE_SUBSAMPLING_RATIO)
    return;
  else
    samplingIteration = 0;

  const bool decisiveBit = signalHistory[decisiveBitPosition];

  if (awaitEndBit)
    return awaitEndOfMessage(decisiveBit, &outputByte, &isStartBitReceived, &numberOfBitsRead, &awaitEndBit, &isSamplingStarted);

  if (!isStartBitReceived)
    return awaitStartOfMessage(decisiveBit, &isStartBitReceived);

  readBit(&outputByte, decisiveBit, &numberOfBitsRead, &awaitEndBit);

  // Serial.print("Has read bit ");
  // Serial.println(numberOfBitsRead);

  // if (receivedSignal)
  //   Serial.print("Reading 1\n");
  // else
  //   Serial.print("Reading 0\n");
}

void handleLogs()
{
}

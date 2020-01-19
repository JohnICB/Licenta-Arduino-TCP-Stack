#define INPUT_PIN 7
#define OUTPUT_PIN 8
#define TASK_LENGTH_MS 70000
#define RECEIVE_SUBSAMPLING_RATIO 10
#define MESSAGE_BIT_LENGTH 56 // 7*8

const unsigned int decisiveBitPosition = floor((RECEIVE_SUBSAMPLING_RATIO * 80) / 100);
// unsigned char payloadToSend = 'f'; // * d42	0b 0010 1010  --> 0101 0100

// size seq  data
// byte byte size*bytes
byte writeBuffer[7] = {7, 20, 'I', 'o', 'n', 'u', 't'};
bool signalHistory[RECEIVE_SUBSAMPLING_RATIO];

void handleTx();
void handleRx();
void handleLogs();

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(OUTPUT_PIN, OUTPUT);
  pinMode(INPUT_PIN, INPUT);

  for (int i = 0; i < RECEIVE_SUBSAMPLING_RATIO; i++)
    signalHistory[i] = 0;
}

void loop()
{
  static unsigned long startTime = micros();
  static unsigned long endTime = micros();
  static float CPULoad = 0;

  const unsigned long currentTime = micros();

  if (currentTime - endTime > TASK_LENGTH_MS)
  {
    handleTx();
    handleRx();
    handleLogs();

    startTime = currentTime;
    endTime = micros();
    CPULoad = (endTime - startTime) / TASK_LENGTH_MS;
    CPULoad++;
  }
}

void sendEndBit(unsigned int *iterationsToSkip, bool *flagEndOfMessage, bool *isStartBitSent)
{
  digitalWrite(OUTPUT_PIN, HIGH);

  *iterationsToSkip = 5;
  *flagEndOfMessage = false;
  *isStartBitSent = false;
}

void sendStartBit(bool *isStartBitSent)
{
  digitalWrite(OUTPUT_PIN, HIGH);
  *isStartBitSent = true;

  // Serial.print("Sent start bit\n");
}

void sendNextBit(unsigned int *bitIndex, bool *flagEndOfMessage)
{
  static unsigned int byteIndex = 0;
  const bool signalToSend = (writeBuffer[byteIndex] >> *bitIndex) & 1; // what bit to send

  *bitIndex = *bitIndex + 1;

  digitalWrite(OUTPUT_PIN, signalToSend);
  // Serial.print("Sending -> ");
  // Serial.println(signalToSend);

  if (*bitIndex == 8)
  {

    // Serial.print(" Byte: ");
    // Serial.print(writeBuffer[byteIndex]);
    // Serial.print("  Byte Index: ");
    // Serial.println(byteIndex);

    *bitIndex = 0;
    byteIndex++;

    if (byteIndex == MESSAGE_BIT_LENGTH / 8)
    {
      *flagEndOfMessage = true; // if we finished transferring the bits from the character flag it so we send start bit again when we send the next message
    }
  }
}

void handleTx()
{
  static unsigned int callIteration = 0;
  static unsigned int bitIndex = 0;
  static unsigned int iterationsToSkip = 0;

  static bool isStartBitSent = false;
  static bool flagEndOfMessage = false;
  static bool eot = false;

  callIteration++;

  if (callIteration < RECEIVE_SUBSAMPLING_RATIO) // send next signal when the time has come - - - - - now 0 - - - - - now 1 - - - - - where - is a skipped iteration but rx is happening
    return;

  callIteration = 0;

  if (eot)
    return digitalWrite(OUTPUT_PIN, 0);

  if (flagEndOfMessage)
  {
    eot = true;
    return sendEndBit(&iterationsToSkip, &flagEndOfMessage, &isStartBitSent);
  }

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

  sendNextBit(&bitIndex, &flagEndOfMessage);
}

void awaitEndOfMessage(bool *isStartBitReceived, bool *awaitEndBit, bool *isSamplingStarted)
{
  *isStartBitReceived = 0;
  *awaitEndBit = false;
  *isSamplingStarted = false;
}

void awaitStartOfMessage(bool *isStartBitReceived)
{
  *isStartBitReceived = true;
}

void readBit(bool decisiveBit, bool *awaitEndBit)
{
  static unsigned int bitIndex = 0;
  static unsigned int byteIndex = 0;
  static byte outputByte = 0;

  static byte readBuffer[7];

  outputByte = (outputByte) | (byte)(decisiveBit << bitIndex++);
  // Serial.print("Read ->");
  // Serial.println(decisiveBit);
  Serial.print(decisiveBit);

  if (bitIndex == 8)
  {
    bitIndex = 0;

    readBuffer[byteIndex] = outputByte;

    Serial.print("\nByte: ");
    Serial.print(readBuffer[byteIndex]);
    Serial.print("  Byte Index: ");
    Serial.println(byteIndex);

    outputByte = 0;
    byteIndex++;

    if (byteIndex == MESSAGE_BIT_LENGTH / 8)
    {
      *awaitEndBit = true;

      Serial.print("Size read: ");
      Serial.println(readBuffer[0]);
      Serial.print("SEQ read: ");
      Serial.println(readBuffer[1]);
      Serial.print("Message read: ");

      for (int i = 2; i < MESSAGE_BIT_LENGTH / 8; i++)
        Serial.print((char)readBuffer[i]);

      Serial.println("\n------------------------------------");

      byteIndex = 0;
    }
  }
}

void handleRx()
{
  static unsigned int samplingIteration = 0; // counts function calls

  static bool isStartBitReceived = false;
  static bool isSamplingStarted = false;
  static bool awaitEndBit = false;

  const bool receivedSignal = digitalRead(INPUT_PIN);

  if (!isSamplingStarted)
  {
    if (receivedSignal)
    {
      isSamplingStarted = true;
      samplingIteration = 0;
    }
    else
      return;
  }

  samplingIteration++;

  signalHistory[samplingIteration - 1] = receivedSignal;

  // Serial.print("#");
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

  if (awaitEndBit && decisiveBit)
    return awaitEndOfMessage(&isStartBitReceived, &awaitEndBit, &isSamplingStarted);

  if (!isStartBitReceived && decisiveBit)
  {
    isStartBitReceived = true;
    return;
  }

  readBit(decisiveBit, &awaitEndBit);
}

void handleLogs()
{
}

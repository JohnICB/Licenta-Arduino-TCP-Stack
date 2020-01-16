#define INPUT_PIN 7
#define OUTPUT_PIN 13
#define FREQUENCY_MS 200
#define RECEIVE_SUBSAMPLING_RATIO 10
#define MESSAGE_BIT_LENGTH 8

const unsigned int decisiveBitPosition = floor((RECEIVE_SUBSAMPLING_RATIO * 80) / 100);
const char payloadToSend = '*'; // d42	0b 0010 1010

bool isStartBitSent;   // make static
bool receivedSignal;   // make static
bool flagEndOfMessage; // make static

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

  isStartBitSent = false;
  flagEndOfMessage = false;

  receivedSignal = 0;

  for (int i = 0; i < RECEIVE_SUBSAMPLING_RATIO; i++)
    signalHistory[i] = 0;
}

void loop()
{
  static unsigned long endTime = millis();
  const unsigned long currentTime = millis();

  if (currentTime - endTime > FREQUENCY_MS)
  {
    handleTx();
    handleRx();
    handleLogs();

    endTime = millis();
  }
}

void sendEndBit(int *iterationsToSkip)
{
  digitalWrite(OUTPUT_PIN, HIGH);

  *iterationsToSkip = 5;
  *flagEndOfMessage = true;

  Serial.print("\nSent end bit\n");
}

void sendStartBit(bool *flagEndOfMessage)
{
  digitalWrite(OUTPUT_PIN, HIGH);

  Serial.print("Sent start bit\n");
}

void sendNextBit(bool *isStartBitSent, unsigned int *bitIndex, bool *flagEndOfMessage)
{
  *isStartBitSent = false;

  const bool signalToSend = (payloadToSend >> *bitIndex) & 1; // what bit to send

  digitalWrite(OUTPUT_PIN, signalToSend);

  *bitIndex++; // = (*bitIndex + 1) % (MESSAGE_BIT_LENGTH - 1);

  if (*bitIndex == MESSAGE_BIT_LENGTH)
  {
    *bitIndex = 0;
    *flagEndOfMessage = true; // if we finished transferring the bits from the character flag it so we send start bit again when we send the next message
  }

  // if (signalToSend)
  //   Serial.print("Sending 1\n");
  // else
  //   Serial.print("Sending 0\n");

  // if (*flagEndOfMessage)
  // Serial.print("****\n");
}

void handleTx()
{
  static unsigned int callIteration = 0;
  static unsigned int bitIndex = 0;
  static unsigned int iterationsToSkip = 0;

  static bool flagEndOfMessage = false;

  callIteration++;

  if (callIteration < RECEIVE_SUBSAMPLING_RATIO) // send next signal when the time has come - - - - - now 0 - - - - - now 1 - - - - - where - is a skipped iteration but rx is happening
    return;

  callIteration = 0;

  if (flagEndOfMessage)
  {
    flagEndOfMessage = false;
    return sendEndBit(&iterationsToSkip);
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
    return sendStartBit(&flagEndOfMessage);

  return sendNextBit(&isStartBitSent, &bitIndex, &flagEndOfMessage);
}

void handleRx()
{
  static unsigned int numberOfBitsRead = 0;  // counts the number of bits read
  static unsigned int samplingIteration = 0; // counts function calls

  static char payloadRead = 0;

  static bool isStartBitReceived = false;
  static bool awaitEndBit = false;

  samplingIteration++; // = (samplingIteration + 1) % RECEIVE_SUBSAMPLING_RATIO;

  receivedSignal = digitalRead(OUTPUT_PIN); // TODO make this IN
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

  Serial.print(decisiveBit);
  Serial.print(" -> rec -> now: ");

  if (awaitEndBit)
  {
    if (!decisiveBit)
      return;

    Serial.print("\nReceived END bit\nMessage read:");
    Serial.println(payloadRead, BIN);
    Serial.println(payloadRead);

    payloadRead = 0;
    isStartBitReceived = 0;
    numberOfBitsRead = 0;
    awaitEndBit = false;

    return;
  }

  if (!isStartBitReceived)
  {
    if (decisiveBit)
    {
      isStartBitReceived = true;
      Serial.print("Received start bit\n");
    }

    return;
  }

  payloadRead = (payloadRead << 1) | decisiveBit;
  Serial.println(payloadRead, BIN);

  numberOfBitsRead++;

  if (numberOfBitsRead == MESSAGE_BIT_LENGTH)
    awaitEndBit = true;

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

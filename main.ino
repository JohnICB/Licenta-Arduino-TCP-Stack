#define INPUT_PIN 7
#define OUTPUT_PIN 13
#define FREQUENCY_MS 500
#define RECEIVE_SUBSAMPLING_RATIO 10

const unsigned int decisiveBitPosition = floor((RECEIVE_SUBSAMPLING_RATIO * 80) / 100);
const char payloadToSend = '*'; // d42	0b 0010 1010

bool isStartBitSent;   // make static
bool sentSignal;       // make static
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

  sentSignal = 0;
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

void handleTx()
{
  static unsigned int callIteration = 0;
  static unsigned int bitIndex = 0;

  if (callIteration < RECEIVE_SUBSAMPLING_RATIO) // send next signal when the time has come - - - - - now 0 - - - - - now 1 - - - - - where - is a skipped iteration but rx is happening
  {
    callIteration++;
    return;
  }

  callIteration = callIteration % RECEIVE_SUBSAMPLING_RATIO;

  if (bitIndex == 0 && isStartBitSent == false) // check if we must send start bit
  {
    digitalWrite(OUTPUT_PIN, HIGH);
    isStartBitSent = true;

    Serial.print("Sent start bit\n");
    callIteration++;

    return;
  }

  isStartBitSent = false;

  sentSignal = (payloadToSend >> bitIndex) & 1; // what bit to send

  digitalWrite(OUTPUT_PIN, sentSignal);

  bitIndex = (bitIndex + 1) % 8;

  // if we finished transferring the bits from the character flag it so we send start bit again when we send the next message
  if (bitIndex == 0)
    flagEndOfMessage = true;
  else
    flagEndOfMessage = false;

  if (sentSignal)
    Serial.print("Sending 1\n");
  else
    Serial.print("Sending 0\n");

  if (flagEndOfMessage)
    Serial.print("****\n");

  callIteration++;
}

void handleRx()
{
  static unsigned int bufferIndex = 0;
  static unsigned int samplingIteration = 0;

  static char payloadRead = 0;

  static bool startedReading = false;

  receivedSignal = digitalRead(OUTPUT_PIN); // TODO make this IN
  signalHistory[samplingIteration] = receivedSignal;

  Serial.print(samplingIteration);
  Serial.print(" > ");

  for (int i = 0; i < RECEIVE_SUBSAMPLING_RATIO; i++)
  {
    Serial.print(signalHistory[i]);
    Serial.print(", ");
  }
  Serial.print("\n");

  if (samplingIteration < RECEIVE_SUBSAMPLING_RATIO - 1)
  {
    samplingIteration++;
    return;
  }
  else
    samplingIteration = 0;

  const bool decisiveBit = signalHistory[decisiveBitPosition];

  Serial.print("Decisive bit: ");
  Serial.print(decisiveBit);
  Serial.print("\n ");

  if (!startedReading && decisiveBit)
  {
    startedReading = true;

    Serial.print("Received start bit\n");

    return;
  }

  // TODO: Handle end of message
  payloadRead = (payloadRead << 1) & decisiveBit;

  // if (receivedSignal)
  //   Serial.print("Reading 1\n");
  // else
  //   Serial.print("Reading 0\n");
}

void handleLogs()
{
}

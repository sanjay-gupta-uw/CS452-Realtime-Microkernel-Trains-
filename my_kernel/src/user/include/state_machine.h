// state machine for CTS quirk
enum class STATES
{
    CTS_LOW,
    CTS_HIGH,
    TX_HIGH,
    TOTAL_STATES
};
#define NUM_STATES (int)STATES::TOTAL_STATES
class TransmitMachine
{
    bool isTransmitting;
    bool hasCTSGoneDown;

public:
    TransmitMachine();
    ~TransmitMachine();
    void update_state(STATES state);
    void reset();
    bool isReady();
    void begin_transmission();
};

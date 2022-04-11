#ifndef LEDBlinker_h
#define LEDBlinker_h

class LEDBlinker
{
  public:
    LEDBlinker(int pin);
    void invertState();
  private:
    int _pin;
    bool _isOn;
};

#endif
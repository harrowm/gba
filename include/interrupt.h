#ifndef INTERRUPT_H
#define INTERRUPT_H

class InterruptController {
private:
    bool vBlank;
    bool hBlank;

public:
    void triggerInterrupt();
};

#endif

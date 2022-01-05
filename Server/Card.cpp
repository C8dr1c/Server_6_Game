#include "Card.h"

Card::Card(int value) : value_(value) {}

int Card::getValue() const {
    return value_;
}

int Card::getPoints() const {
    if (value_ == 55) {
        return 7;
    }
    else if (value_ % 11 == 0) {
        return 5;
    }
    else if (value_ % 10 == 0) {
        return 3;
    }
    else if (value_ % 5 == 0) {
        return 2;
    }
    else {
        return 1;
    }
}
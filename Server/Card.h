#ifndef CARD_H
#define CARD_H

class Card {

public:
    Card(int value);

    int getValue() const;
    int getPoints() const;

private:
    int value_;
};
#endif
#ifndef ELEMENT_DATA_H
#define ELEMENT_DATA_H

#include <string>
#include <vector>

struct ElementInfo {
    int atomicNumber;
    int neutronNumber;
    std::string symbol;
    std::string name;
};

class ElementData {
public:
    static const std::vector<ElementInfo>& supportedElements();
    static ElementInfo byAtomicNumber(int atomicNumber);
};

#endif // ELEMENT_DATA_H

#include "ElementData.h"

const std::vector<ElementInfo>& ElementData::supportedElements() {
    static const std::vector<ElementInfo> elements = {
        {1, 0, "H", "Hydrogen"},
        {2, 2, "He", "Helium"},
        {6, 6, "C", "Carbon"},
        {8, 8, "O", "Oxygen"},
        {10, 10, "Ne", "Neon"}
    };
    return elements;
}

ElementInfo ElementData::byAtomicNumber(int atomicNumber) {
    const auto& elements = supportedElements();
    for (const auto& element : elements) {
        if (element.atomicNumber == atomicNumber) {
            return element;
        }
    }
    return elements.front();
}

#include <gui/containers/CustomGauge.hpp>

CustomGauge::CustomGauge() {

}

void CustomGauge::initialize() {
	CustomGaugeBase::initialize();
}

void CustomGauge::changeValue(int8_t scrollAmount) {
	int16_t newValue = value + scrollAmount * rotationSpeed;
	if (newValue < 0)
		newValue = 0;
	else if (newValue > 255)
		newValue = 255;

	value = newValue;

	float arcAngle = -127.5 + value;

	activeCircle.setArc(-127.5, arcAngle);
	invalidate();
}

void CustomGauge::setValue(uint8_t newValue) {
	value = newValue;

	float arcAngle = -127.5 + value;

	activeCircle.setArc(-127.5, arcAngle);
	invalidate();
}

uint8_t CustomGauge::getValue() {
	return value;
}

void CustomGauge::setParamName(TEXTS paramName) {
	Unicode::snprintf(parameterNameBuffer, PARAMETERNAME_SIZE, "%s",
			touchgfx::TypedText(paramName).getText());
	parameterName.invalidate();
}

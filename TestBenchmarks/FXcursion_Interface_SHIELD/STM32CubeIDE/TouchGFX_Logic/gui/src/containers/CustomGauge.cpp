#include <gui/containers/CustomGauge.hpp>

CustomGauge::CustomGauge()
{

}

void CustomGauge::initialize()
{
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

/*
 * Names now come from g_aFxDesc, shared byte for byte with the audio
 * controller and the VST, so they are plain C strings rather than TEXTS ids.
 *
 * That takes them out of the TouchGFX text database and therefore out of
 * translation, which is the right trade: a parameter name has to match what
 * the DSP calls it, and these are technical terms rather than prose.
 *
 * fromUTF8 truncates rather than overruns. PARAMETERNAME_SIZE is 10 while
 * FX_PARAM_DESC documents names as up to 11 characters, so there are two
 * characters of slack between the contract and the buffer - the longest name
 * in the pool today is 9. HostTests/test_cfgmap.c asserts the bound so a
 * longer name is a failing test rather than a clipped label on the screen.
 */
void CustomGauge::setParamName(const char* pName) {
	if (0 == pName)
	{
		parameterNameBuffer[0] = 0;
	}
	else
	{
		Unicode::fromUTF8((const uint8_t*)pName,
		                  parameterNameBuffer,
		                  PARAMETERNAME_SIZE);
	}

	parameterName.invalidate();
}

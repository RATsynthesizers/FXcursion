#include <gui/fxchain_screen/FXchainView.hpp>
#include <gui/fxchain_screen/FXchainPresenter.hpp>

FXchainPresenter::FXchainPresenter(FXchainView &v) :
		view(v) {

}

void FXchainPresenter::activate() {

}

void FXchainPresenter::deactivate() {

}

void FXchainPresenter::parameterChange(uint8_t id, int8_t scrollAmount) {
	view.parameterChange(id, scrollAmount);
}

void FXchainPresenter::encScroll_action(int8_t scrollAmount) {
	view.encScroll_action(scrollAmount);
}

void FXchainPresenter::btnYES_action(void) {
	view.btnYES_action();
}

void FXchainPresenter::btnNO_action(void) {
	view.btnNO_action();
}


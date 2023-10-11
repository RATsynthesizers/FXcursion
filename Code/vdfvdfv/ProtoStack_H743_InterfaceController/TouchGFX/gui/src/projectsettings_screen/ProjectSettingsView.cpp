#include <gui/projectsettings_screen/ProjectSettingsView.hpp>
#include <touchgfx/Color.hpp>
#include <cmath>

ProjectSettingsView::ProjectSettingsView() {

}

void ProjectSettingsView::setupScreen() {
	ProjectSettingsViewBase::setupScreen();
	currentPos = presenter->getSettingsPosition();

	ioArray[currentPos]->selectItem();
	projectName = presenter->getProjectName();
	Unicode::fromUTF8(projectName, projectSettings_textBuffer,
			PROJECTSETTINGS_TEXT_SIZE);

	for(int i = 0; i < WIRES_NUM; i++)
	{
		lines[i]->setVisible(presenter->getWire(i));
		lines[i]->invalidate();
	}

	for (int i = 0; i < CHANNELS_NUM; i++) {
		const uint8_t io[6] = "";
		if (i < 3) {
			strcpy((char*) io, "In ");
			char buf[2];
			sprintf(buf, "%d", i + 1);
			strcat((char*) io, buf);
		} else {
			strcpy((char*) io, "Out ");
			char buf[2];
			sprintf(buf, "%d", (i - 3) + 1);
			strcat((char*) io, buf);
		}

		ioArray[i]->setName(io);
	}
}

void ProjectSettingsView::tearDownScreen() {
	ProjectSettingsViewBase::tearDownScreen();
}

void ProjectSettingsView::parameterChange(uint8_t id, int8_t scrollAmount) {

}

void ProjectSettingsView::encScroll_action(int8_t scrollAmount) {
	if (isSelectingAction) {
		subSettingsContainer.scrollContents(scrollAmount);
	} else if (connectDelete == NO_ACTION) {
		ioArray[currentPos]->deselectItem();
		currentPos = currentPos + scrollAmount
				- 6 * floor((currentPos + scrollAmount) / 6.);
		ioArray[currentPos]->selectItem();
	} else {
		int newCurrentPos;
		int newSelectedLineNumber;

		for (int i = 0; i < 2; i++) {
			newCurrentPos = 3 * (currentPos >= 3) + currentPos
					+ scrollAmount * (i + 1)
					- 3 * floor((currentPos + scrollAmount * (i + 1)) / 3.);

			if (currentPos < 3)
				newSelectedLineNumber = (connectingPos % 3)
						+ 3 * (newCurrentPos % 3);
			else
				newSelectedLineNumber = (connectingPos % 3) * 3
						+ (newCurrentPos % 3);

			if ((connectDelete == CONNECT)
					!= lines[newSelectedLineNumber]->isVisible()) {

				ioArray[currentPos]->deselectItem();
				ioArray[currentPos]->deselectConnection();

				if (connectDelete == CONNECT)
					lines[selectedLineNumber]->setVisible(false);
				else
					linePainters[selectedLineNumber]->setColor(
							touchgfx::Color::getColorFromRGB(0, 0, 0));
				lines[selectedLineNumber]->invalidate();

				selectedLineNumber = newSelectedLineNumber;

				linePainters[selectedLineNumber]->setColor(
						touchgfx::Color::getColorFromRGB(255, 255, 255));
				lines[selectedLineNumber]->setVisible(true);
				lines[selectedLineNumber]->invalidate();

				currentPos = newCurrentPos;
				ioArray[currentPos]->selectItem();
				ioArray[currentPos]->selectConnection();
				break;
			}
		}
	}
}

void ProjectSettingsView::btnYES_action(void) {

	if (connectDelete == NO_ACTION) {
		if (!isSelectingAction) {

			bool grayDeleteWire = true;
			bool grayCreateWire = true;

			if (currentPos < 3) {
				for (int i = currentPos * 3; i < currentPos * 3 + 3; i++)
					if (lines[i]->isVisible()) {
						grayDeleteWire = false;
						break;
					}
				for (int i = currentPos * 3; i < currentPos * 3 + 3; i++)
					if (!lines[i]->isVisible()) {
						grayCreateWire = false;
						break;
					}
			} else {
				for (int i = currentPos % 3; i < 9; i += 3)
					if (lines[i]->isVisible()) {
						grayDeleteWire = false;
						break;
					}
				for (int i = currentPos % 3; i < 9; i += 3)
					if (!lines[i]->isVisible()) {
						grayCreateWire = false;
						break;
					}
			}
			subSettingsContainer.resetSubMenu(currentPos, grayDeleteWire,
					grayCreateWire);

			isSelectingAction = true;
			subSettingsContainer.setVisible(isSelectingAction);
			subSettingsContainer.invalidate();

		} else {
			uint8_t subMenuCurrentPos = subSettingsContainer.getCurrentPos();
			switch (subMenuCurrentPos) {
			case 0:
				presenter->saveSettingsPosition(currentPos);
				presenter->saveChannelNum(currentPos);
				for(int i = 0; i < WIRES_NUM; i++)
					presenter->saveWire(i, lines[i]->isVisible());

				//TODO: COVER transition
				application().gotoFXchainScreenBlockTransition();
				break;
			case 1:
				connectDelete = CONNECT;

				isSelectingAction = false;
				subSettingsContainer.setVisible(isSelectingAction);
				subSettingsContainer.invalidate();

				wiringChangeBox.setText((uint8_t) connectDelete, currentPos);
				wiringChangeBox.setVisible(true);
				wiringChangeBox.invalidate();

				connectingPos = currentPos;
				ioArray[connectingPos]->deselectItem();
				ioArray[connectingPos]->selectConnection();
				if (connectingPos < 3) {
					for (int i = connectingPos * 3; i < connectingPos * 3 + 3;
							i++)
						if (!lines[i]->isVisible()) {
							selectedLineNumber = i;
							linePainters[i]->setColor(
									touchgfx::Color::getColorFromRGB(255, 255,
											255));
							lines[i]->setVisible(true);
							lines[i]->invalidate();

							currentPos = i % 3 + 3;
							ioArray[currentPos]->selectItem();
							ioArray[currentPos]->selectConnection();
							break;
						}
				} else {
					for (int i = connectingPos % 3; i < 9; i += 3) {
						if (!lines[i]->isVisible()) {
							selectedLineNumber = i;
							linePainters[i]->setColor(
									touchgfx::Color::getColorFromRGB(255, 255,
											255));
							lines[i]->setVisible(true);
							lines[i]->invalidate();

							currentPos = i / 3;
							ioArray[currentPos]->selectItem();
							ioArray[currentPos]->selectConnection();
							break;
						}
					}
				}
				break;
			case 2:
				connectDelete = DELETE;

				isSelectingAction = false;
				subSettingsContainer.setVisible(isSelectingAction);
				subSettingsContainer.invalidate();

				wiringChangeBox.setText((uint8_t) connectDelete, currentPos);
				wiringChangeBox.setVisible(true);
				wiringChangeBox.invalidate();

				connectingPos = currentPos;
				ioArray[connectingPos]->deselectItem();
				ioArray[connectingPos]->selectConnection();
				if (connectingPos < 3) {
					for (int i = connectingPos * 3; i < connectingPos * 3 + 3;
							i++)
						if (lines[i]->isVisible()) {
							selectedLineNumber = i;
							linePainters[i]->setColor(
									touchgfx::Color::getColorFromRGB(255, 255,
											255));
							lines[i]->invalidate();

							currentPos = i % 3 + 3;
							ioArray[currentPos]->selectItem();
							ioArray[currentPos]->selectConnection();
							break;
						}
				} else {
					for (int i = connectingPos % 3; i < 9; i += 3) {
						if (lines[i]->isVisible()) {
							selectedLineNumber = i;
							linePainters[i]->setColor(
									touchgfx::Color::getColorFromRGB(255, 255,
											255));
							lines[i]->invalidate();

							currentPos = i / 3;
							ioArray[currentPos]->selectItem();
							ioArray[currentPos]->selectConnection();
							break;
						}
					}
				}
				break;
			default:
				break;
			}
		}

	} else {
		ioArray[connectingPos]->deselectConnection();
		ioArray[currentPos]->deselectItem();
		ioArray[currentPos]->deselectConnection();

		if (connectDelete == CONNECT) {
			linePainters[selectedLineNumber]->setColor(
					touchgfx::Color::getColorFromRGB(0, 0, 0));
			lines[selectedLineNumber]->invalidate();

		} else {
			lines[selectedLineNumber]->setVisible(false);
			lines[selectedLineNumber]->invalidate();
		}

		connectDelete = NO_ACTION;

		wiringChangeBox.setVisible(false);
		wiringChangeBox.invalidate();

		currentPos = connectingPos;
		ioArray[currentPos]->selectItem();
	}

}

void ProjectSettingsView::btnNO_action(void) {
//TODO: action on btnNO
	if (connectDelete == NO_ACTION) {
		if (isSelectingAction) {
			isSelectingAction = false;
			subSettingsContainer.setVisible(isSelectingAction);
			subSettingsContainer.invalidate();
		}
	} else {

		wiringChangeBox.setVisible(false);
		wiringChangeBox.invalidate();

		ioArray[currentPos]->deselectItem();
		ioArray[currentPos]->deselectConnection();

		if (connectDelete == CONNECT) {
			lines[selectedLineNumber]->setVisible(false);
			lines[selectedLineNumber]->invalidate();
		} else {
			linePainters[selectedLineNumber]->setColor(
					touchgfx::Color::getColorFromRGB(0, 0, 0));
			lines[selectedLineNumber]->invalidate();
		}

		connectDelete = NO_ACTION;

		currentPos = connectingPos;

		ioArray[currentPos]->selectItem();
		ioArray[currentPos]->deselectConnection();
	}
}

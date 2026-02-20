#include <gui/system_screen/SystemView.hpp>

const U8 SystemView::MIXER_X_POSITIONS[4] =
{
	218,
	160,
	102,
	44
};


SystemView::SystemView()
{
	bIsMixerAdded = FALSE;
	eMixerPosition = CHAIN_MODULE_1;

	inputType.bIsStereo1 = FALSE;
	inputType.bIsStereo2 = FALSE;

	ePrevSelect 	= SELECT_INPUT;
	eCurrentSelect 	= SELECT_INPUT;
}

void SystemView::setupScreen()
{
	inputType.bIsStereo1 = presenter->getInputIsStereo1();
	inputType.bIsStereo2 = presenter->getInputIsStereo2();

	ModuleName addModule;
	if(TRUE == inputType.bIsStereo1)
	{
		for(U8 j = 0; j < 4; j++)
		{
			addModule = presenter->getStereoModuleInPosition(0, j);
			if(MODULE_NONE != addModule)
			{
				stereoChain1.addModule((ChainModuleNumber) j, addModule);
			}
		}
	}
	else
	{
		for(U8 j = 0; j < 4; j++)
		{
			addModule = presenter->getMonoModuleInPosition(0, j);
			if(MODULE_NONE != addModule)
			{
				monoChain1.addModule((ChainModuleNumber) j, addModule);
			}

			addModule = presenter->getMonoModuleInPosition(1, j);
			if(MODULE_NONE != addModule)
			{
				monoChain2.addModule((ChainModuleNumber) j, addModule);
			}
		}
	}
	if(TRUE == inputType.bIsStereo2)
	{
		for(U8 j = 0; j < 4; j++)
		{
			addModule = presenter->getStereoModuleInPosition(1, j);
			if(MODULE_NONE != addModule)
			{
				stereoChain2.addModule((ChainModuleNumber) j, addModule);
			}
		}
	}
	else
	{
		for(U8 j = 0; j < 4; j++)
		{
			addModule = presenter->getMonoModuleInPosition(2, j);
			if(MODULE_NONE != addModule)
			{
				monoChain3.addModule((ChainModuleNumber) j, addModule);
			}

			addModule = presenter->getMonoModuleInPosition(3, j);
			if(MODULE_NONE != addModule)
			{
				monoChain4.addModule((ChainModuleNumber) j, addModule);
			}
		}
	}

	S8 nMixerPosition = presenter->getMixerPosition();

	if(-1 == nMixerPosition)
	{
		bIsMixerAdded = FALSE;
	}
	else
	{
		bIsMixerAdded = TRUE;
		eMixerPosition = (ChainModuleNumber) nMixerPosition;
		mixModule.setXY(MIXER_X_POSITIONS[eMixerPosition], MIXER_Y_POS);
		mixModule.setVisible(true);
	}

	eCurrentSelect = presenter->getSelectedModule();
	ePrevSelect = presenter->getPrevSelectedModule();

	switch(eCurrentSelect)
	{
	default:
	case SELECT_INPUT:
		inModule.select();
		break;
	case SELECT_OUTPUT:
		outModule.select();
		break;
	case SELECT_MONO_CHAIN_1:
	case SELECT_MONO_CHAIN_2:
	case SELECT_MONO_CHAIN_3:
	case SELECT_MONO_CHAIN_4:
		if(bIsMixerAdded
			&& eMixerPosition == presenter->getSelectedChainModule())
		{
			mixModule.select();
		}
		monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->select(presenter->getSelectedChainModule());
		break;
	case SELECT_STEREO_CHAIN_1:
	case SELECT_STEREO_CHAIN_2:
		if(bIsMixerAdded
			&& eMixerPosition == presenter->getSelectedChainModule())
		{
			mixModule.select();
		}
		stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->select(presenter->getSelectedChainModule());
		break;
	case SELECT_STOMP_BOARD:
		stompBoard.select(presenter->getSelectedFootSwitch());
		break;
	}

    SystemViewBase::setupScreen();
}

void SystemView::tearDownScreen()
{
    SystemViewBase::tearDownScreen();
}

void SystemView::encMenuUpdate(S8 nValue)
{
	if(true == addModuleWindow.isVisible())
	{
		if(1 == nValue)
		{
			addModuleWindow.selectDown();
		}
		else
		{
			addModuleWindow.selectUp();
		}
	}
	else
	{
		switch(eCurrentSelect)
		{
		case SELECT_INPUT:

			if(-1 == nValue)
			{
				switch(ePrevSelect)
				{
				case SELECT_MONO_CHAIN_1:
				default:

					if(FALSE == inputType.bIsStereo1)
					{
						eCurrentSelect = SELECT_MONO_CHAIN_1;
						monoChain1.select(CHAIN_MODULE_1);
					}
					else
					{
						eCurrentSelect = SELECT_STEREO_CHAIN_1;
						stereoChain1.select(CHAIN_MODULE_1);
					}
					break;

				case SELECT_MONO_CHAIN_2:

					if(FALSE == inputType.bIsStereo1)
					{
						eCurrentSelect = SELECT_MONO_CHAIN_2;
						monoChain2.select(CHAIN_MODULE_1);
					}
					else
					{
						eCurrentSelect = SELECT_STEREO_CHAIN_1;
						stereoChain1.select(CHAIN_MODULE_1);
					}
					break;

				case SELECT_MONO_CHAIN_3:

					if(FALSE == inputType.bIsStereo2)
					{
						eCurrentSelect = SELECT_MONO_CHAIN_3;
						monoChain3.select(CHAIN_MODULE_1);
					}
					else
					{
						eCurrentSelect = SELECT_STEREO_CHAIN_2;
						stereoChain2.select(CHAIN_MODULE_1);
					}
					break;

				case SELECT_MONO_CHAIN_4:

					if(FALSE == inputType.bIsStereo2)
					{
						eCurrentSelect = SELECT_MONO_CHAIN_4;
						monoChain4.select(CHAIN_MODULE_1);
					}
					else
					{
						eCurrentSelect = SELECT_STEREO_CHAIN_2;
						stereoChain2.select(CHAIN_MODULE_1);
					}
					break;
				}

				ePrevSelect = SELECT_INPUT;
				inModule.deselect();

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_1 == eMixerPosition)
				{
					mixModule.select();
				}

			}

			break;

		case SELECT_OUTPUT:

			if(1 == nValue)
			{
				switch(ePrevSelect)
				{
				case SELECT_MONO_CHAIN_1:
				default:

					if(FALSE == inputType.bIsStereo1)
					{
						eCurrentSelect = SELECT_MONO_CHAIN_1;
						monoChain1.select(CHAIN_MODULE_4);
					}
					else
					{
						eCurrentSelect = SELECT_STEREO_CHAIN_1;
						stereoChain1.select(CHAIN_MODULE_4);
					}
					break;

				case SELECT_MONO_CHAIN_2:

					if(FALSE == inputType.bIsStereo1)
					{
						eCurrentSelect = SELECT_MONO_CHAIN_2;
						monoChain2.select(CHAIN_MODULE_4);
					}
					else
					{
						eCurrentSelect = SELECT_STEREO_CHAIN_1;
						stereoChain1.select(CHAIN_MODULE_4);
					}
					break;

				case SELECT_MONO_CHAIN_3:

					if(FALSE == inputType.bIsStereo2)
					{
						eCurrentSelect = SELECT_MONO_CHAIN_3;
						monoChain3.select(CHAIN_MODULE_4);
					}
					else
					{
						eCurrentSelect = SELECT_STEREO_CHAIN_2;
						stereoChain2.select(CHAIN_MODULE_4);
					}
					break;

				case SELECT_MONO_CHAIN_4:

					if(FALSE == inputType.bIsStereo2)
					{
						eCurrentSelect = SELECT_MONO_CHAIN_4;
						monoChain4.select(CHAIN_MODULE_4);
					}
					else
					{
						eCurrentSelect = SELECT_STEREO_CHAIN_2;
						stereoChain2.select(CHAIN_MODULE_4);
					}
					break;
				}

				ePrevSelect = SELECT_OUTPUT;
				outModule.deselect();

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_4 == eMixerPosition)
				{
					mixModule.select();
				}
			}


			break;

		case SELECT_MONO_CHAIN_1:

			switch(monoChain1.getSelectedModuleNumber())
			{
			case CHAIN_MODULE_1:

				monoChain1.deselect(CHAIN_MODULE_1);

				if(1 == nValue)
				{
					ePrevSelect = SELECT_MONO_CHAIN_1;

					eCurrentSelect = SELECT_INPUT;
					inModule.select();

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_1 == eMixerPosition)
					{
						mixModule.deselect();
					}
				}
				else
				{
					monoChain1.select(CHAIN_MODULE_2);

					if(TRUE == bIsMixerAdded)
					{
						if(CHAIN_MODULE_2 == eMixerPosition)
						{
							mixModule.select();
						}
						else if (CHAIN_MODULE_1 == eMixerPosition)
						{
							mixModule.deselect();
						}
					}
				}
				break;

			case CHAIN_MODULE_2:

				monoChain1.deselect(CHAIN_MODULE_2);

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_2 == eMixerPosition)
				{
					mixModule.deselect();
				}

				if(1 == nValue)
				{
					monoChain1.select(CHAIN_MODULE_1);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_1 == eMixerPosition)
					{
						mixModule.select();
					}
				}
				else
				{
					monoChain1.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_3 == eMixerPosition)
					{
						mixModule.select();
					}
				}

				break;

			case CHAIN_MODULE_3:

				monoChain1.deselect(CHAIN_MODULE_3);

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_3 == eMixerPosition)
				{
					mixModule.deselect();
				}

				if(1 == nValue)
				{
					monoChain1.select(CHAIN_MODULE_2);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_2 == eMixerPosition)
					{
						mixModule.select();
					}
				}
				else
				{
					monoChain1.select(CHAIN_MODULE_4);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_4 == eMixerPosition)
					{
						mixModule.select();
					}
				}

				break;

			case CHAIN_MODULE_4:

				monoChain1.deselect(CHAIN_MODULE_4);

				if(1 == nValue)
				{
					monoChain1.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded)
					{
						if(CHAIN_MODULE_3 == eMixerPosition)
						{
							mixModule.select();
						}
						else if (CHAIN_MODULE_4 == eMixerPosition)
						{
							mixModule.deselect();
						}
					}
				}
				else
				{
					ePrevSelect = SELECT_MONO_CHAIN_1;

					eCurrentSelect = SELECT_OUTPUT;
					outModule.select();

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_4 == eMixerPosition)
					{
						mixModule.deselect();
					}
				}

				break;

			default:
				break;
			}
			break;

		case SELECT_MONO_CHAIN_2:

			switch(monoChain2.getSelectedModuleNumber())
			{
			case CHAIN_MODULE_1:

				monoChain2.deselect(CHAIN_MODULE_1);

				if(1 == nValue)
				{
					ePrevSelect = SELECT_MONO_CHAIN_2;

					eCurrentSelect = SELECT_INPUT;
					inModule.select();

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_1 == eMixerPosition)
					{
						mixModule.deselect();
					}
				}
				else
				{
					monoChain2.select(CHAIN_MODULE_2);

					if(TRUE == bIsMixerAdded)
					{
						if(CHAIN_MODULE_2 == eMixerPosition)
						{
							mixModule.select();
						}
						else if (CHAIN_MODULE_1 == eMixerPosition)
						{
							mixModule.deselect();
						}
					}
				}
				break;

			case CHAIN_MODULE_2:

				monoChain2.deselect(CHAIN_MODULE_2);

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_2 == eMixerPosition)
				{
					mixModule.deselect();
				}

				if(1 == nValue)
				{
					monoChain2.select(CHAIN_MODULE_1);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_1 == eMixerPosition)
					{
						mixModule.select();
					}
				}
				else
				{
					monoChain2.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_3 == eMixerPosition)
					{
						mixModule.select();
					}
				}

				break;

			case CHAIN_MODULE_3:

				monoChain2.deselect(CHAIN_MODULE_3);

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_3 == eMixerPosition)
				{
					mixModule.deselect();
				}

				if(1 == nValue)
				{
					monoChain2.select(CHAIN_MODULE_2);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_2 == eMixerPosition)
					{
						mixModule.select();
					}
				}
				else
				{
					monoChain2.select(CHAIN_MODULE_4);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_4 == eMixerPosition)
					{
						mixModule.select();
					}
				}

				break;

			case CHAIN_MODULE_4:

				monoChain2.deselect(CHAIN_MODULE_4);

				if(1 == nValue)
				{
					monoChain2.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded)
					{
						if(CHAIN_MODULE_3 == eMixerPosition)
						{
							mixModule.select();
						}
						else if (CHAIN_MODULE_4 == eMixerPosition)
						{
							mixModule.deselect();
						}
					}
				}
				else
				{
					ePrevSelect = SELECT_MONO_CHAIN_2;

					eCurrentSelect = SELECT_OUTPUT;
					outModule.select();

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_4 == eMixerPosition)
					{
						mixModule.deselect();
					}
				}

				break;

			default:
				break;
			}
			break;

		case SELECT_MONO_CHAIN_3:

			switch(monoChain3.getSelectedModuleNumber())
			{
			case CHAIN_MODULE_1:

				monoChain3.deselect(CHAIN_MODULE_1);

				if(1 == nValue)
				{
					ePrevSelect = SELECT_MONO_CHAIN_3;

					eCurrentSelect = SELECT_INPUT;
					inModule.select();

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_1 == eMixerPosition)
					{
						mixModule.deselect();
					}
				}
				else
				{
					monoChain3.select(CHAIN_MODULE_2);

					if(TRUE == bIsMixerAdded)
					{
						if(CHAIN_MODULE_2 == eMixerPosition)
						{
							mixModule.select();
						}
						else if (CHAIN_MODULE_1 == eMixerPosition)
						{
							mixModule.deselect();
						}
					}
				}
				break;

			case CHAIN_MODULE_2:

				monoChain3.deselect(CHAIN_MODULE_2);

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_2 == eMixerPosition)
				{
					mixModule.deselect();
				}

				if(1 == nValue)
				{
					monoChain3.select(CHAIN_MODULE_1);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_1 == eMixerPosition)
					{
						mixModule.select();
					}
				}
				else
				{
					monoChain3.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_3 == eMixerPosition)
					{
						mixModule.select();
					}
				}

				break;

			case CHAIN_MODULE_3:

				monoChain3.deselect(CHAIN_MODULE_3);

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_3 == eMixerPosition)
				{
					mixModule.deselect();
				}

				if(1 == nValue)
				{
					monoChain3.select(CHAIN_MODULE_2);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_2 == eMixerPosition)
					{
						mixModule.select();
					}
				}
				else
				{
					monoChain3.select(CHAIN_MODULE_4);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_4 == eMixerPosition)
					{
						mixModule.select();
					}
				}

				break;

			case CHAIN_MODULE_4:

				monoChain3.deselect(CHAIN_MODULE_4);

				if(1 == nValue)
				{
					monoChain3.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded)
					{
						if(CHAIN_MODULE_3 == eMixerPosition)
						{
							mixModule.select();
						}
						else if (CHAIN_MODULE_4 == eMixerPosition)
						{
							mixModule.deselect();
						}
					}
				}
				else
				{
					ePrevSelect = SELECT_MONO_CHAIN_3;

					eCurrentSelect = SELECT_OUTPUT;
					outModule.select();

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_4 == eMixerPosition)
					{
						mixModule.deselect();
					}
				}

				break;

			default:
				break;
			}
			break;

		case SELECT_MONO_CHAIN_4:

			switch(monoChain4.getSelectedModuleNumber())
			{
			case CHAIN_MODULE_1:

				monoChain4.deselect(CHAIN_MODULE_1);

				if(1 == nValue)
				{
					ePrevSelect = SELECT_MONO_CHAIN_4;

					eCurrentSelect = SELECT_INPUT;
					inModule.select();

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_1 == eMixerPosition)
					{
						mixModule.deselect();
					}
				}
				else
				{
					monoChain4.select(CHAIN_MODULE_2);

					if(TRUE == bIsMixerAdded)
					{
						if(CHAIN_MODULE_2 == eMixerPosition)
						{
							mixModule.select();
						}
						else if (CHAIN_MODULE_1 == eMixerPosition)
						{
							mixModule.deselect();
						}
					}
				}
				break;

			case CHAIN_MODULE_2:

				monoChain4.deselect(CHAIN_MODULE_2);

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_2 == eMixerPosition)
				{
					mixModule.deselect();
				}

				if(1 == nValue)
				{
					monoChain4.select(CHAIN_MODULE_1);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_1 == eMixerPosition)
					{
						mixModule.select();
					}
				}
				else
				{
					monoChain4.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_3 == eMixerPosition)
					{
						mixModule.select();
					}
				}

				break;

			case CHAIN_MODULE_3:

				monoChain4.deselect(CHAIN_MODULE_3);

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_3 == eMixerPosition)
				{
					mixModule.deselect();
				}

				if(1 == nValue)
				{
					monoChain4.select(CHAIN_MODULE_2);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_2 == eMixerPosition)
					{
						mixModule.select();
					}
				}
				else
				{
					monoChain4.select(CHAIN_MODULE_4);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_4 == eMixerPosition)
					{
						mixModule.select();
					}
				}

				break;

			case CHAIN_MODULE_4:

				monoChain4.deselect(CHAIN_MODULE_4);

				if(1 == nValue)
				{
					monoChain4.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded)
					{
						if(CHAIN_MODULE_3 == eMixerPosition)
						{
							mixModule.select();
						}
						else if (CHAIN_MODULE_4 == eMixerPosition)
						{
							mixModule.deselect();
						}
					}
				}
				else
				{
					ePrevSelect = SELECT_MONO_CHAIN_4;

					eCurrentSelect = SELECT_OUTPUT;
					outModule.select();

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_4 == eMixerPosition)
					{
						mixModule.deselect();
					}
				}

				break;

			default:
				break;
			}
			break;

		case SELECT_STEREO_CHAIN_1:

			switch(stereoChain1.getSelectedModuleNumber())
			{
			case CHAIN_MODULE_1:

				stereoChain1.deselect(CHAIN_MODULE_1);

				if(1 == nValue)
				{
					ePrevSelect = SELECT_STEREO_CHAIN_1;

					eCurrentSelect = SELECT_INPUT;
					inModule.select();

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_1 == eMixerPosition)
					{
						mixModule.deselect();
					}
				}
				else
				{
					stereoChain1.select(CHAIN_MODULE_2);

					if(TRUE == bIsMixerAdded)
					{
						if(CHAIN_MODULE_2 == eMixerPosition)
						{
							mixModule.select();
						}
						else if (CHAIN_MODULE_1 == eMixerPosition)
						{
							mixModule.deselect();
						}
					}
				}
				break;

			case CHAIN_MODULE_2:

				stereoChain1.deselect(CHAIN_MODULE_2);

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_2 == eMixerPosition)
				{
					mixModule.deselect();
				}

				if(1 == nValue)
				{
					stereoChain1.select(CHAIN_MODULE_1);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_1 == eMixerPosition)
					{
						mixModule.select();
					}
				}
				else
				{
					stereoChain1.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_3 == eMixerPosition)
					{
						mixModule.select();
					}
				}

				break;

			case CHAIN_MODULE_3:

				stereoChain1.deselect(CHAIN_MODULE_3);

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_3 == eMixerPosition)
				{
					mixModule.deselect();
				}

				if(1 == nValue)
				{
					stereoChain1.select(CHAIN_MODULE_2);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_2 == eMixerPosition)
					{
						mixModule.select();
					}
				}
				else
				{
					stereoChain1.select(CHAIN_MODULE_4);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_4 == eMixerPosition)
					{
						mixModule.select();
					}
				}

				break;

			case CHAIN_MODULE_4:

				stereoChain1.deselect(CHAIN_MODULE_4);

				if(1 == nValue)
				{
					stereoChain1.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded)
					{
						if(CHAIN_MODULE_3 == eMixerPosition)
						{
							mixModule.select();
						}
						else if (CHAIN_MODULE_4 == eMixerPosition)
						{
							mixModule.deselect();
						}
					}
				}
				else
				{
					ePrevSelect = SELECT_STEREO_CHAIN_1;

					eCurrentSelect = SELECT_OUTPUT;
					outModule.select();

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_4 == eMixerPosition)
					{
						mixModule.deselect();
					}
				}

				break;

			default:
				break;
			}
			break;

		case SELECT_STEREO_CHAIN_2:

			switch(stereoChain2.getSelectedModuleNumber())
			{
			case CHAIN_MODULE_1:

				stereoChain2.deselect(CHAIN_MODULE_1);

				if(1 == nValue)
				{
					ePrevSelect = SELECT_STEREO_CHAIN_2;

					eCurrentSelect = SELECT_INPUT;
					inModule.select();

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_1 == eMixerPosition)
					{
						mixModule.deselect();
					}
				}
				else
				{
					stereoChain2.select(CHAIN_MODULE_2);

					if(TRUE == bIsMixerAdded)
					{
						if(CHAIN_MODULE_2 == eMixerPosition)
						{
							mixModule.select();
						}
						else if (CHAIN_MODULE_1 == eMixerPosition)
						{
							mixModule.deselect();
						}
					}
				}
				break;

			case CHAIN_MODULE_2:

				stereoChain2.deselect(CHAIN_MODULE_2);

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_2 == eMixerPosition)
				{
					mixModule.deselect();
				}

				if(1 == nValue)
				{
					stereoChain2.select(CHAIN_MODULE_1);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_1 == eMixerPosition)
					{
						mixModule.select();
					}
				}
				else
				{
					stereoChain2.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_3 == eMixerPosition)
					{
						mixModule.select();
					}
				}

				break;

			case CHAIN_MODULE_3:

				stereoChain2.deselect(CHAIN_MODULE_3);

				if(TRUE == bIsMixerAdded
						&& CHAIN_MODULE_3 == eMixerPosition)
				{
					mixModule.deselect();
				}

				if(1 == nValue)
				{
					stereoChain2.select(CHAIN_MODULE_2);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_2 == eMixerPosition)
					{
						mixModule.select();
					}
				}
				else
				{
					stereoChain2.select(CHAIN_MODULE_4);

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_4 == eMixerPosition)
					{
						mixModule.select();
					}
				}

				break;

			case CHAIN_MODULE_4:

				stereoChain2.deselect(CHAIN_MODULE_4);

				if(1 == nValue)
				{
					stereoChain2.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded)
					{
						if(CHAIN_MODULE_3 == eMixerPosition)
						{
							mixModule.select();
						}
						else if (CHAIN_MODULE_4 == eMixerPosition)
						{
							mixModule.deselect();
						}
					}
				}
				else
				{
					ePrevSelect = SELECT_STEREO_CHAIN_2;

					eCurrentSelect = SELECT_OUTPUT;
					outModule.select();

					if(TRUE == bIsMixerAdded
							&& CHAIN_MODULE_4 == eMixerPosition)
					{
						mixModule.deselect();
					}
				}

				break;

			default:
				break;
			}
			break;

		case SELECT_STOMP_BOARD:

			switch(stompBoard.getSelectedFootSwitch())
			{
			case FOOT_SWITCH_1:

				if(1 == nValue)
				{
					stompBoard.select(FOOT_SWITCH_2);
				}
				break;

			case FOOT_SWITCH_2:

				if(1 == nValue)
				{
					stompBoard.select(FOOT_SWITCH_3);
				}
				else
				{
					stompBoard.select(FOOT_SWITCH_1);
				}
				break;

			case FOOT_SWITCH_3:

				if(-1 == nValue)
				{
					stompBoard.select(FOOT_SWITCH_2);
				}
				break;

			default:
				break;
			}
			break;

		default:
			break;
		}
	}
}

void SystemView::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	// Button pressed
	if(1 == nValue)
	{
		if(TRUE == modalWindowDelete.isVisible())
		{
			switch(eCurrentSelect)
			{
			case SELECT_MONO_CHAIN_1:
			case SELECT_MONO_CHAIN_2:
			case SELECT_MONO_CHAIN_3:
			case SELECT_MONO_CHAIN_4:
				if(TRUE == bIsMixerAdded
						&& eMixerPosition == monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber())
				{
					addModuleWindow.unblockSelect(MODULE_MIX);
					mixModule.setVisible(false);
					mixModule.deselect();
					monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->select(
							monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber());
					bIsMixerAdded = FALSE;

					presenter->saveMixerPosition(-1);
				}
				else
				{
					addModuleWindow.unblockSelect(monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleName());
					monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->deleteSelectedModule();

					presenter->saveMonoModulePosition(MODULE_NONE, 0, monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber());
				}

				presenter->clearFXChain((ChannelType) (eCurrentSelect - SELECT_MONO_CHAIN_1));
				break;

			case SELECT_STEREO_CHAIN_1:
			case SELECT_STEREO_CHAIN_2:
				if(TRUE == bIsMixerAdded
						&& eMixerPosition == stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber())
				{
					addModuleWindow.unblockSelect(MODULE_MIX);
					mixModule.setVisible(false);
					mixModule.deselect();
					stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->select(
							stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber());
					bIsMixerAdded = FALSE;

					presenter->saveMixerPosition(-1);
				}
				else
				{
					addModuleWindow.unblockSelect(stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleName());
					stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->deleteSelectedModule();

					presenter->saveStereoModulePosition(MODULE_NONE, 0, stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber());
				}

				presenter->clearFXChain((ChannelType) (eCurrentSelect - SELECT_MONO_CHAIN_1));
				break;

			default:
				break;
			}

			modalWindowDelete.setVisible(FALSE);
			modalWindowDelete.invalidate();
		}
		else if(TRUE == addModuleWindow.isVisible())
		{
			if(MODULE_MIX == addModuleWindow.getAddModuleName())
			{
				switch(eCurrentSelect)
				{
				case SELECT_MONO_CHAIN_1:
				case SELECT_MONO_CHAIN_2:
				case SELECT_MONO_CHAIN_3:
				case SELECT_MONO_CHAIN_4:
					eMixerPosition = monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber();
					presenter->saveSelectedChannel(
							(ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule(
							(ChainModuleNumber) monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber());
					break;
				case SELECT_STEREO_CHAIN_1:
				case SELECT_STEREO_CHAIN_2:
					eMixerPosition = stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber();
					presenter->saveSelectedChannel(
							(ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule(
							(ChainModuleNumber) stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber());

					break;
				default:
					break;
				}
				presenter->saveMixerPosition(eMixerPosition);
				application().gotoMixerScreenNoTransition();
			}
			else
			{
				switch(eCurrentSelect)
				{
				case SELECT_MONO_CHAIN_1:
				case SELECT_MONO_CHAIN_2:
				case SELECT_MONO_CHAIN_3:
				case SELECT_MONO_CHAIN_4:

					monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->addModule(
							monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber(),
							addModuleWindow.getAddModuleName());
					presenter->saveMonoModulePosition(
							addModuleWindow.getAddModuleName(),
							eCurrentSelect - SELECT_MONO_CHAIN_1,
							monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber());
					presenter->saveSelectedChannel(
							(ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule(
							(ChainModuleNumber) monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber());

					switch(addModuleWindow.getAddModuleName())
					{
					case MODULE_FX:
						application().gotoFXChainScreenNoTransition();
						break;
					case MODULE_LOOP:
						application().gotoLooperScreenNoTransition();
						break;
					case MODULE_REC:
						application().gotoRecorderScreenNoTransition();
						break;
					default:
						break;
					}
					break;
				case SELECT_STEREO_CHAIN_1:
				case SELECT_STEREO_CHAIN_2:
					stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->addModule(
							stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber(),
							addModuleWindow.getAddModuleName());
					presenter->saveStereoModulePosition(
							addModuleWindow.getAddModuleName(),
							eCurrentSelect - SELECT_STEREO_CHAIN_1,
							stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber());
					presenter->saveSelectedChannel(
							(ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule(
							(ChainModuleNumber) stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber());

					switch(addModuleWindow.getAddModuleName())
					{
					case MODULE_FX:
						application().gotoFXChainScreenNoTransition();
						break;
					case MODULE_LOOP:
						application().gotoLooperScreenNoTransition();
						break;
					case MODULE_REC:
						application().gotoRecorderScreenNoTransition();
						break;
					default:
						break;
					}
					break;
				default:
					break;
				}
			}
		}
		else
		{
			ModuleName eSelectedModuleName = MODULE_NONE;
			S8 eSelectedModuleNumber = -1;

			ModuleName aCurrentChainModules[4] = {};

			switch(eCurrentSelect)
			{
			case SELECT_MONO_CHAIN_1:
				eSelectedModuleName = monoChain1.getSelectedModuleName();
				eSelectedModuleNumber = monoChain1.getSelectedModuleNumber();

				for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
				{
					aCurrentChainModules[i] = monoChain1.getModuleName(i);
				}

				break;
			case SELECT_MONO_CHAIN_2:
				eSelectedModuleName = monoChain2.getSelectedModuleName();
				eSelectedModuleNumber = monoChain2.getSelectedModuleNumber();

				for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
				{
					aCurrentChainModules[i] = monoChain2.getModuleName(i);
				}

				break;
			case SELECT_MONO_CHAIN_3:
				eSelectedModuleName = monoChain3.getSelectedModuleName();
				eSelectedModuleNumber = monoChain3.getSelectedModuleNumber();

				for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
				{
					aCurrentChainModules[i] = monoChain3.getModuleName(i);
				}

				break;
			case SELECT_MONO_CHAIN_4:
				eSelectedModuleName = monoChain4.getSelectedModuleName();
				eSelectedModuleNumber = monoChain4.getSelectedModuleNumber();

				for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
				{
					aCurrentChainModules[i] = monoChain4.getModuleName(i);
				}

				break;
			case SELECT_STEREO_CHAIN_1:
				eSelectedModuleName = stereoChain1.getSelectedModuleName();
				eSelectedModuleNumber = stereoChain1.getSelectedModuleNumber();

				for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
				{
					aCurrentChainModules[i] = stereoChain1.getModuleName(i);
				}

				break;
			case SELECT_STEREO_CHAIN_2:
				eSelectedModuleName = stereoChain2.getSelectedModuleName();
				eSelectedModuleNumber = stereoChain2.getSelectedModuleNumber();

				for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
				{
					aCurrentChainModules[i] = stereoChain2.getModuleName(i);
				}

				break;
			case SELECT_INPUT:
				eSelectedModuleName = MODULE_INPUT;
				break;
			case SELECT_OUTPUT:
				eSelectedModuleName = MODULE_OUTPUT;
				break;
			case SELECT_STOMP_BOARD:
				eSelectedModuleName = MODULE_STOMP;
				eSelectedModuleNumber = stompBoard.getSelectedFootSwitch();
				break;
			default:
				break;
			}

			presenter->saveSelectedModule(eCurrentSelect);
			presenter->savePrevSelectedModule(ePrevSelect);

			if(TRUE == bIsMixerAdded
				&& eMixerPosition == eSelectedModuleNumber
				&& SELECT_STOMP_BOARD != eCurrentSelect)
			{
				presenter->saveSelectedChainModule((ChainModuleNumber) eSelectedModuleNumber);
				application().gotoMixerScreenNoTransition();
			}
			else
			{
				switch(eSelectedModuleName)
				{
				case MODULE_NONE:

					for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
					{
						if(MODULE_NONE != aCurrentChainModules[i])
						{
							addModuleWindow.blockSelect(aCurrentChainModules[i]);
						}
					}

					if(TRUE == bIsMixerAdded
						|| MODULE_NONE != monoChain1.getModuleName((ChainModuleNumber) eSelectedModuleNumber)
						|| MODULE_NONE != monoChain2.getModuleName((ChainModuleNumber) eSelectedModuleNumber)
						|| MODULE_NONE != monoChain3.getModuleName((ChainModuleNumber) eSelectedModuleNumber)
						|| MODULE_NONE != monoChain4.getModuleName((ChainModuleNumber) eSelectedModuleNumber)
						|| MODULE_NONE != stereoChain1.getModuleName((ChainModuleNumber) eSelectedModuleNumber)
						|| MODULE_NONE != stereoChain2.getModuleName((ChainModuleNumber) eSelectedModuleNumber))
					{
						addModuleWindow.blockSelect(MODULE_MIX);
					}

					addModuleWindow.setVisible(TRUE);
					addModuleWindow.selectFirst();
					addModuleWindow.invalidate();

					break;
				case MODULE_INPUT:
					application().gotoInputScreenNoTransition();
					break;
				case MODULE_OUTPUT:
					application().gotoOutputScreenNoTransition();
					break;
				case MODULE_FX:
					presenter->saveSelectedChannel((ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule((ChainModuleNumber) eSelectedModuleNumber);
					application().gotoFXChainScreenNoTransition();
					break;
				case MODULE_LOOP:
					presenter->saveSelectedChannel((ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule((ChainModuleNumber) eSelectedModuleNumber);
					application().gotoLooperScreenNoTransition();
					break;
				case MODULE_REC:
					presenter->saveSelectedChannel((ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule((ChainModuleNumber) eSelectedModuleNumber);
					application().gotoRecorderScreenNoTransition();
					break;
				case MODULE_STOMP:
					presenter->saveSelectedFootSwitch((FootSwitches) eSelectedModuleNumber);
					application().gotoStompScreenNoTransition();
					break;
				default:
					break;
				}
			}
		}
	}
	else if(2 == nValue)	// Button long press
	{

	}
}

void SystemView::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	// Button pressed
	if(1 == nValue)
	{
		if(TRUE == modalWindowDelete.isVisible())
		{
			modalWindowDelete.setVisible(FALSE);
			modalWindowDelete.invalidate();
		}
		else if(TRUE == addModuleWindow.isVisible())
		{
			addModuleWindow.unblockSelect(MODULE_FX);
			addModuleWindow.unblockSelect(MODULE_REC);
			addModuleWindow.unblockSelect(MODULE_LOOP);
			addModuleWindow.unblockSelect(MODULE_MIX);

			addModuleWindow.setVisible(FALSE);
			addModuleWindow.invalidate();
		}
		else
		{
			if(TRUE == bIsFuncPressed)
			{
				U8 modalText[20] = {0};
				switch(eCurrentSelect)
				{
				case SELECT_MONO_CHAIN_1:
				case SELECT_MONO_CHAIN_2:
				case SELECT_MONO_CHAIN_3:
				case SELECT_MONO_CHAIN_4:
					if(TRUE == bIsMixerAdded
							&& eMixerPosition == monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber())
					{
						sprintf((char*)modalText, "Mixer");
						modalWindowDelete.setText(modalText);
						modalWindowDelete.setVisible(TRUE);
						modalWindowDelete.invalidate();
					}
					else
					{
						switch(monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleName())
						{
						case MODULE_FX:
							sprintf((char*)modalText, "Mono %d %s", (eCurrentSelect - SELECT_MONO_CHAIN_1) + 1, "FX chain");
							modalWindowDelete.setText(modalText);
							modalWindowDelete.setVisible(TRUE);
							modalWindowDelete.invalidate();
							break;
						case MODULE_LOOP:
							sprintf((char*)modalText, "Mono %d %s", (eCurrentSelect - SELECT_MONO_CHAIN_1) + 1, "Looper");
							modalWindowDelete.setText(modalText);
							modalWindowDelete.setVisible(TRUE);
							modalWindowDelete.invalidate();
							break;
						case MODULE_REC:
							sprintf((char*)modalText, "Mono %d %s", (eCurrentSelect - SELECT_MONO_CHAIN_1) + 1, "Recorder");
							modalWindowDelete.setText(modalText);
							modalWindowDelete.setVisible(TRUE);
							modalWindowDelete.invalidate();
							break;
						default:
							break;
						}
					}
					break;

				case SELECT_STEREO_CHAIN_1:
				case SELECT_STEREO_CHAIN_2:
					if(TRUE == bIsMixerAdded
							&& eMixerPosition == stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber())
					{
						sprintf((char*)modalText, "Mixer");
						modalWindowDelete.setText(modalText);
						modalWindowDelete.setVisible(TRUE);
						modalWindowDelete.invalidate();
					}
					else
					{
						switch(stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleName())
						{
						case MODULE_FX:
							sprintf((char*)modalText, "Stereo %d %s", (eCurrentSelect - SELECT_STEREO_CHAIN_1) + 1, "FX chain");
							modalWindowDelete.setText(modalText);
							modalWindowDelete.setVisible(TRUE);
							modalWindowDelete.invalidate();
							break;
						case MODULE_LOOP:
							sprintf((char*)modalText, "Stereo %d %s", (eCurrentSelect - SELECT_STEREO_CHAIN_1) + 1, "Looper");
							modalWindowDelete.setText(modalText);
							modalWindowDelete.setVisible(TRUE);
							modalWindowDelete.invalidate();
							break;
						case MODULE_REC:
							sprintf((char*)modalText, "Stereo %d %s", (eCurrentSelect - SELECT_STEREO_CHAIN_1) + 1, "Recorder");
							modalWindowDelete.setText(modalText);
							modalWindowDelete.setVisible(TRUE);
							modalWindowDelete.invalidate();
							break;
						default:
							break;
						}
					}
					break;

				default:
					break;
				}
			}
		}
	}
	else if(2 == nValue)	// Button long press
	{

	}
}

void SystemView::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	if(true == modalWindowDelete.isVisible())
	{
		do_nothing();
	}
	else if(true == addModuleWindow.isVisible())
	{
		addModuleWindow.selectUp();
	}
	else
	{
		switch(eCurrentSelect)
		{
		case SELECT_MONO_CHAIN_2:

			if(FALSE == bIsMixerAdded
					|| eMixerPosition != monoChain2.getSelectedModuleNumber())
			{
				ePrevSelect = SELECT_MONO_CHAIN_2;
				monoChain2.deselect(monoChain2.getSelectedModuleNumber());

				eCurrentSelect = SELECT_MONO_CHAIN_1;
				monoChain1.select(monoChain2.getSelectedModuleNumber());
			}

			break;

		case SELECT_MONO_CHAIN_3:

			if(FALSE == bIsMixerAdded
					|| eMixerPosition != monoChain3.getSelectedModuleNumber())
			{
				ePrevSelect = SELECT_MONO_CHAIN_3;
				monoChain3.deselect(monoChain3.getSelectedModuleNumber());

				if(FALSE == inputType.bIsStereo1)
				{
					eCurrentSelect = SELECT_MONO_CHAIN_2;
					monoChain2.select(monoChain3.getSelectedModuleNumber());
				}
				else
				{
					eCurrentSelect = SELECT_STEREO_CHAIN_1;
					stereoChain1.select(monoChain3.getSelectedModuleNumber());
				}
			}

			break;

		case SELECT_MONO_CHAIN_4:

			if(FALSE == bIsMixerAdded
					|| eMixerPosition != monoChain4.getSelectedModuleNumber())
			{
				ePrevSelect = SELECT_MONO_CHAIN_4;
				monoChain4.deselect(monoChain4.getSelectedModuleNumber());

				eCurrentSelect = SELECT_MONO_CHAIN_3;
				monoChain3.select(monoChain4.getSelectedModuleNumber());
			}

			break;

		case SELECT_STEREO_CHAIN_2:

			if(FALSE == bIsMixerAdded
					|| eMixerPosition != stereoChain2.getSelectedModuleNumber())
			{
				ePrevSelect = SELECT_STEREO_CHAIN_2;
				stereoChain2.deselect(stereoChain2.getSelectedModuleNumber());

				if(FALSE == inputType.bIsStereo1)
				{
					eCurrentSelect = SELECT_MONO_CHAIN_2;
					monoChain2.select(stereoChain2.getSelectedModuleNumber());
				}
				else
				{
					eCurrentSelect = SELECT_STEREO_CHAIN_1;
					stereoChain1.select(stereoChain2.getSelectedModuleNumber());
				}
			}

			break;

		case SELECT_STOMP_BOARD:

			switch(stompBoard.getSelectedFootSwitch())
			{
			case FOOT_SWITCH_1:

//				if(SELECT_OUTPUT == ePrevSelect)
//				{
//					eCurrentSelect = SELECT_OUTPUT;
//					outModule.select();
//				}
//				else
//				{
					if(FALSE == inputType.bIsStereo1)
					{
						eCurrentSelect = SELECT_MONO_CHAIN_4;
						monoChain4.select(CHAIN_MODULE_4);

						if(TRUE == bIsMixerAdded
								&& eMixerPosition == monoChain4.getSelectedModuleNumber())
						{
							mixModule.select();
						}
					}
					else
					{
						eCurrentSelect = SELECT_STEREO_CHAIN_2;
						stereoChain2.select(CHAIN_MODULE_4);

						if(TRUE == bIsMixerAdded
								&& eMixerPosition == stereoChain2.getSelectedModuleNumber())
						{
							mixModule.select();
						}
					}
//				}

				break;
			case FOOT_SWITCH_2:

				if(FALSE == inputType.bIsStereo1)
				{
					eCurrentSelect = SELECT_MONO_CHAIN_4;
					monoChain4.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded
							&& eMixerPosition == monoChain4.getSelectedModuleNumber())
					{
						mixModule.select();
					}
				}
				else
				{
					eCurrentSelect = SELECT_STEREO_CHAIN_2;
					stereoChain2.select(CHAIN_MODULE_3);

					if(TRUE == bIsMixerAdded
							&& eMixerPosition == stereoChain2.getSelectedModuleNumber())
					{
						mixModule.select();
					}
				}

				break;
			case FOOT_SWITCH_3:

//				if(SELECT_INPUT == ePrevSelect)
//				{
//					eCurrentSelect = SELECT_INPUT;
//					inModule.select();
//				}
//				else
//				{
					if(FALSE == inputType.bIsStereo1)
					{
						eCurrentSelect = SELECT_MONO_CHAIN_4;
						monoChain4.select(CHAIN_MODULE_1);

						if(TRUE == bIsMixerAdded
								&& eMixerPosition == monoChain4.getSelectedModuleNumber())
						{
							mixModule.select();
						}
					}
					else
					{
						eCurrentSelect = SELECT_STEREO_CHAIN_2;
						stereoChain2.select(CHAIN_MODULE_1);

						if(TRUE == bIsMixerAdded
								&& eMixerPosition == stereoChain2.getSelectedModuleNumber())
						{
							mixModule.select();
						}
					}
//				}

				break;
			}

			ePrevSelect = SELECT_STOMP_BOARD;
			stompBoard.deselect();

			break;

		case SELECT_INPUT:
		case SELECT_OUTPUT:
		case SELECT_MONO_CHAIN_1:
		case SELECT_STEREO_CHAIN_1:
		default:
			break;
		}
	}
}

void SystemView::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	if(true == modalWindowDelete.isVisible())
	{
		do_nothing();
	}
	else if(true == addModuleWindow.isVisible())
	{
		addModuleWindow.selectDown();
	}
	else
	{
		switch(eCurrentSelect)
		{
		case SELECT_INPUT:

			ePrevSelect = SELECT_INPUT;
			inModule.deselect();

			eCurrentSelect = SELECT_STOMP_BOARD;
			stompBoard.select(FOOT_SWITCH_3);

			break;

		case SELECT_OUTPUT:

			ePrevSelect = SELECT_OUTPUT;
			outModule.deselect();

			eCurrentSelect = SELECT_STOMP_BOARD;
			stompBoard.select(FOOT_SWITCH_1);

			break;

		case SELECT_MONO_CHAIN_1:

			ePrevSelect = SELECT_MONO_CHAIN_1;
			monoChain1.deselect(monoChain1.getSelectedModuleNumber());

			if(TRUE == bIsMixerAdded
					&& eMixerPosition == monoChain1.getSelectedModuleNumber())
			{
				mixModule.deselect();

				eCurrentSelect = SELECT_STOMP_BOARD;
				switch(eMixerPosition)
				{
				case CHAIN_MODULE_1:
					stompBoard.select(FOOT_SWITCH_3);
					break;
				case CHAIN_MODULE_2:
				case CHAIN_MODULE_3:
					stompBoard.select(FOOT_SWITCH_2);
					break;
				case CHAIN_MODULE_4:
					stompBoard.select(FOOT_SWITCH_1);
					break;
				}
			}
			else
			{
				eCurrentSelect = SELECT_MONO_CHAIN_2;
				monoChain2.select(monoChain1.getSelectedModuleNumber());
			}

			break;

		case SELECT_MONO_CHAIN_2:

			ePrevSelect = SELECT_MONO_CHAIN_2;
			monoChain2.deselect(monoChain2.getSelectedModuleNumber());

			if(TRUE == bIsMixerAdded
					&& eMixerPosition == monoChain2.getSelectedModuleNumber())
			{
				mixModule.deselect();

				eCurrentSelect = SELECT_STOMP_BOARD;
				switch(eMixerPosition)
				{
				case CHAIN_MODULE_1:
					stompBoard.select(FOOT_SWITCH_3);
					break;
				case CHAIN_MODULE_2:
				case CHAIN_MODULE_3:
					stompBoard.select(FOOT_SWITCH_2);
					break;
				case CHAIN_MODULE_4:
					stompBoard.select(FOOT_SWITCH_1);
					break;
				}
			}
			else
			{
				if(FALSE == inputType.bIsStereo2)
				{
					eCurrentSelect = SELECT_MONO_CHAIN_3;
					monoChain3.select(monoChain2.getSelectedModuleNumber());
				}
				else
				{
					eCurrentSelect = SELECT_STEREO_CHAIN_2;
					stereoChain2.select(monoChain2.getSelectedModuleNumber());
				}
			}

			break;

		case SELECT_MONO_CHAIN_3:

			ePrevSelect = SELECT_MONO_CHAIN_3;
			monoChain3.deselect(monoChain3.getSelectedModuleNumber());

			if(TRUE == bIsMixerAdded
					&& eMixerPosition == monoChain3.getSelectedModuleNumber())
			{
				mixModule.deselect();

				eCurrentSelect = SELECT_STOMP_BOARD;
				switch(eMixerPosition)
				{
				case CHAIN_MODULE_1:
					stompBoard.select(FOOT_SWITCH_3);
					break;
				case CHAIN_MODULE_2:
				case CHAIN_MODULE_3:
					stompBoard.select(FOOT_SWITCH_2);
					break;
				case CHAIN_MODULE_4:
					stompBoard.select(FOOT_SWITCH_1);
					break;
				}
			}
			else
			{
				eCurrentSelect = SELECT_MONO_CHAIN_4;
				monoChain4.select(monoChain3.getSelectedModuleNumber());
			}

			break;

		case SELECT_MONO_CHAIN_4:

			eCurrentSelect = SELECT_STOMP_BOARD;
			ePrevSelect = SELECT_MONO_CHAIN_4;
			monoChain4.deselect(monoChain4.getSelectedModuleNumber());

			if(TRUE == bIsMixerAdded
					&& eMixerPosition == monoChain4.getSelectedModuleNumber())
			{
				mixModule.deselect();
			}

			switch(monoChain4.getSelectedModuleNumber())
			{
			case CHAIN_MODULE_1:
				stompBoard.select(FOOT_SWITCH_3);
				break;

			case CHAIN_MODULE_2:
			case CHAIN_MODULE_3:
				stompBoard.select(FOOT_SWITCH_2);
				break;

			case CHAIN_MODULE_4:
				stompBoard.select(FOOT_SWITCH_1);
				break;
			default:
				break;
			}

			break;

		case SELECT_STEREO_CHAIN_1:

			ePrevSelect = SELECT_STEREO_CHAIN_1;
			stereoChain1.deselect(stereoChain1.getSelectedModuleNumber());

			if(TRUE == bIsMixerAdded
					&& eMixerPosition == stereoChain1.getSelectedModuleNumber())
			{
				mixModule.deselect();

				eCurrentSelect = SELECT_STOMP_BOARD;
				switch(eMixerPosition)
				{
				case CHAIN_MODULE_1:
					stompBoard.select(FOOT_SWITCH_3);
					break;
				case CHAIN_MODULE_2:
				case CHAIN_MODULE_3:
					stompBoard.select(FOOT_SWITCH_2);
					break;
				case CHAIN_MODULE_4:
					stompBoard.select(FOOT_SWITCH_1);
					break;
				}
			}
			else
			{
				if(FALSE == inputType.bIsStereo2)
				{
					eCurrentSelect = SELECT_MONO_CHAIN_3;
					monoChain3.select(stereoChain1.getSelectedModuleNumber());
				}
				else
				{
					eCurrentSelect = SELECT_STEREO_CHAIN_2;
					stereoChain2.select(stereoChain1.getSelectedModuleNumber());
				}
			}

			break;

		case SELECT_STEREO_CHAIN_2:

			eCurrentSelect = SELECT_STOMP_BOARD;
			ePrevSelect = SELECT_STEREO_CHAIN_2;
			stereoChain2.deselect(stereoChain2.getSelectedModuleNumber());

			if(TRUE == bIsMixerAdded
					&& eMixerPosition == stereoChain2.getSelectedModuleNumber())
			{
				mixModule.deselect();
			}

			switch(stereoChain2.getSelectedModuleNumber())
			{
			case CHAIN_MODULE_1:
				stompBoard.select(FOOT_SWITCH_1);
				break;

			case CHAIN_MODULE_2:
			case CHAIN_MODULE_3:
				stompBoard.select(FOOT_SWITCH_2);
				break;

			case CHAIN_MODULE_4:
				stompBoard.select(FOOT_SWITCH_3);
				break;
			default:
				break;
			}

			break;

		case SELECT_STOMP_BOARD:
		default:
			break;
		}
	}
}

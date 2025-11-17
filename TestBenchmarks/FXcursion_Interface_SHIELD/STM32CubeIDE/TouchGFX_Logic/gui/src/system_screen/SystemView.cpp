#include <gui/system_screen/SystemView.hpp>

SystemView::SystemView()
{
	inputType.bIsStereo1 = FALSE;
	inputType.bIsStereo2 = FALSE;

	ePrevSelect 	= MODULE_INPUT;
	eCurrentSelect 	= MODULE_INPUT;
}

void SystemView::setupScreen()
{
	inModule.select();
    SystemViewBase::setupScreen();
}

void SystemView::tearDownScreen()
{
    SystemViewBase::tearDownScreen();
}

void SystemView::encMenuUpdate(S8 nValue)
{
	switch(eCurrentSelect)
	{
	case MODULE_INPUT:

		if(-1 == nValue)
		{
			switch(ePrevSelect)
			{
			case MONO_CHAIN_1:
			default:

				if(FALSE == inputType.bIsStereo1)
				{
					eCurrentSelect = MONO_CHAIN_1;
					monoChain1.select(MONO_CHAIN_MODULE_1);
				}
				else
				{
					eCurrentSelect = STEREO_CHAIN_1;
					stereoChain1.select(STEREO_CHAIN_MODULE_1);
				}
				break;

			case MONO_CHAIN_2:

				if(FALSE == inputType.bIsStereo1)
				{
					eCurrentSelect = MONO_CHAIN_2;
					monoChain2.select(MONO_CHAIN_MODULE_1);
				}
				else
				{
					eCurrentSelect = STEREO_CHAIN_1;
					stereoChain1.select(STEREO_CHAIN_MODULE_1);
				}
				break;

			case MONO_CHAIN_3:

				if(FALSE == inputType.bIsStereo2)
				{
					eCurrentSelect = MONO_CHAIN_3;
					monoChain3.select(MONO_CHAIN_MODULE_1);
				}
				else
				{
					eCurrentSelect = STEREO_CHAIN_2;
					stereoChain2.select(STEREO_CHAIN_MODULE_1);
				}
				break;

			case MONO_CHAIN_4:

				if(FALSE == inputType.bIsStereo2)
				{
					eCurrentSelect = MONO_CHAIN_4;
					monoChain4.select(MONO_CHAIN_MODULE_1);
				}
				else
				{
					eCurrentSelect = STEREO_CHAIN_2;
					stereoChain2.select(STEREO_CHAIN_MODULE_1);
				}
				break;
			}

			ePrevSelect = MODULE_INPUT;
			inModule.deselect();
		}

		break;

	case MODULE_OUTPUT:

		if(1 == nValue)
		{
			switch(ePrevSelect)
			{
			case MONO_CHAIN_1:
			default:

				if(FALSE == inputType.bIsStereo1)
				{
					eCurrentSelect = MONO_CHAIN_1;
					monoChain1.select(MONO_CHAIN_MODULE_4);
				}
				else
				{
					eCurrentSelect = STEREO_CHAIN_1;
					stereoChain1.select(STEREO_CHAIN_MODULE_4);
				}
				break;

			case MONO_CHAIN_2:

				if(FALSE == inputType.bIsStereo1)
				{
					eCurrentSelect = MONO_CHAIN_2;
					monoChain2.select(MONO_CHAIN_MODULE_4);
				}
				else
				{
					eCurrentSelect = STEREO_CHAIN_1;
					stereoChain1.select(STEREO_CHAIN_MODULE_4);
				}
				break;

			case MONO_CHAIN_3:

				if(FALSE == inputType.bIsStereo2)
				{
					eCurrentSelect = MONO_CHAIN_3;
					monoChain3.select(MONO_CHAIN_MODULE_4);
				}
				else
				{
					eCurrentSelect = STEREO_CHAIN_2;
					stereoChain2.select(STEREO_CHAIN_MODULE_4);
				}
				break;

			case MONO_CHAIN_4:

				if(FALSE == inputType.bIsStereo2)
				{
					eCurrentSelect = MONO_CHAIN_4;
					monoChain4.select(MONO_CHAIN_MODULE_4);
				}
				else
				{
					eCurrentSelect = STEREO_CHAIN_2;
					stereoChain2.select(STEREO_CHAIN_MODULE_4);
				}
				break;
			}

			ePrevSelect = MODULE_OUTPUT;
			outModule.deselect();
		}


		break;

	case MONO_CHAIN_1:

		switch(monoChain1.getSelectedModule())
		{
		case MONO_CHAIN_MODULE_1:

			if(1 == nValue)
			{
				ePrevSelect = MONO_CHAIN_1;
				monoChain1.deselect();

				eCurrentSelect = MODULE_INPUT;
				inModule.select();
			}
			else
			{
				monoChain1.select(MONO_CHAIN_MODULE_2);
			}
			break;

		case MONO_CHAIN_MODULE_2:

			if(1 == nValue)
			{
				monoChain1.select(MONO_CHAIN_MODULE_1);
			}
			else
			{
				monoChain1.select(MONO_CHAIN_MODULE_3);
			}
			break;

		case MONO_CHAIN_MODULE_3:

			if(1 == nValue)
			{
				monoChain1.select(MONO_CHAIN_MODULE_2);
			}
			else
			{
				monoChain1.select(MONO_CHAIN_MODULE_4);
			}
			break;

		case MONO_CHAIN_MODULE_4:

			if(1 == nValue)
			{
				monoChain1.select(MONO_CHAIN_MODULE_3);
			}
			else
			{
				ePrevSelect = MONO_CHAIN_1;
				monoChain1.deselect();

				eCurrentSelect = MODULE_OUTPUT;
				outModule.select();
			}
			break;

		default:
			break;
		}
		break;

	case MONO_CHAIN_2:

		switch(monoChain2.getSelectedModule())
		{
		case MONO_CHAIN_MODULE_1:

			if(1 == nValue)
			{
				ePrevSelect = MONO_CHAIN_2;
				monoChain2.deselect();

				eCurrentSelect = MODULE_INPUT;
				inModule.select();
			}
			else
			{
				monoChain2.select(MONO_CHAIN_MODULE_2);
			}
			break;

		case MONO_CHAIN_MODULE_2:

			if(1 == nValue)
			{
				monoChain2.select(MONO_CHAIN_MODULE_1);
			}
			else
			{
				monoChain2.select(MONO_CHAIN_MODULE_3);
			}
			break;

		case MONO_CHAIN_MODULE_3:

			if(1 == nValue)
			{
				monoChain2.select(MONO_CHAIN_MODULE_2);
			}
			else
			{
				monoChain2.select(MONO_CHAIN_MODULE_4);
			}
			break;

		case MONO_CHAIN_MODULE_4:

			if(1 == nValue)
			{
				monoChain2.select(MONO_CHAIN_MODULE_3);
			}
			else
			{
				ePrevSelect = MONO_CHAIN_2;
				monoChain2.deselect();

				eCurrentSelect = MODULE_OUTPUT;
				outModule.select();
			}
			break;

		default:
			break;
		}
		break;

	case MONO_CHAIN_3:

		switch(monoChain3.getSelectedModule())
		{
		case MONO_CHAIN_MODULE_1:

			if(1 == nValue)
			{
				ePrevSelect = MONO_CHAIN_3;
				monoChain3.deselect();

				eCurrentSelect = MODULE_INPUT;
				inModule.select();
			}
			else
			{
				monoChain3.select(MONO_CHAIN_MODULE_2);
			}
			break;

		case MONO_CHAIN_MODULE_2:

			if(1 == nValue)
			{
				monoChain3.select(MONO_CHAIN_MODULE_1);
			}
			else
			{
				monoChain3.select(MONO_CHAIN_MODULE_3);
			}
			break;

		case MONO_CHAIN_MODULE_3:

			if(1 == nValue)
			{
				monoChain3.select(MONO_CHAIN_MODULE_2);
			}
			else
			{
				monoChain3.select(MONO_CHAIN_MODULE_4);
			}
			break;

		case MONO_CHAIN_MODULE_4:

			if(1 == nValue)
			{
				monoChain3.select(MONO_CHAIN_MODULE_3);
			}
			else
			{
				ePrevSelect = MONO_CHAIN_3;
				monoChain3.deselect();

				eCurrentSelect = MODULE_OUTPUT;
				outModule.select();
			}
			break;

		default:
			break;
		}
		break;

	case MONO_CHAIN_4:

		switch(monoChain4.getSelectedModule())
		{
		case MONO_CHAIN_MODULE_1:

			if(1 == nValue)
			{
				ePrevSelect = MONO_CHAIN_4;
				monoChain4.deselect();

				eCurrentSelect = MODULE_INPUT;
				inModule.select();
			}
			else
			{
				monoChain4.select(MONO_CHAIN_MODULE_2);
			}
			break;

		case MONO_CHAIN_MODULE_2:

			if(1 == nValue)
			{
				monoChain4.select(MONO_CHAIN_MODULE_1);
			}
			else
			{
				monoChain4.select(MONO_CHAIN_MODULE_3);
			}
			break;

		case MONO_CHAIN_MODULE_3:

			if(1 == nValue)
			{
				monoChain4.select(MONO_CHAIN_MODULE_2);
			}
			else
			{
				monoChain4.select(MONO_CHAIN_MODULE_4);
			}
			break;

		case MONO_CHAIN_MODULE_4:

			if(1 == nValue)
			{
				monoChain4.select(MONO_CHAIN_MODULE_3);
			}
			else
			{
				ePrevSelect = MONO_CHAIN_4;
				monoChain4.deselect();

				eCurrentSelect = MODULE_OUTPUT;
				outModule.select();
			}
			break;

		default:
			break;
		}
		break;

	case STEREO_CHAIN_1:

		switch(stereoChain1.getSelectedModule())
		{
		case STEREO_CHAIN_MODULE_1:

			if(1 == nValue)
			{
				ePrevSelect = STEREO_CHAIN_1;
				stereoChain1.deselect();

				eCurrentSelect = MODULE_INPUT;
				inModule.select();
			}
			else
			{
				stereoChain1.select(STEREO_CHAIN_MODULE_2);
			}
			break;

		case STEREO_CHAIN_MODULE_2:

			if(1 == nValue)
			{
				stereoChain1.select(STEREO_CHAIN_MODULE_1);
			}
			else
			{
				stereoChain1.select(STEREO_CHAIN_MODULE_3);
			}
			break;

		case STEREO_CHAIN_MODULE_3:

			if(1 == nValue)
			{
				stereoChain1.select(STEREO_CHAIN_MODULE_2);
			}
			else
			{
				stereoChain1.select(STEREO_CHAIN_MODULE_4);
			}
			break;

		case STEREO_CHAIN_MODULE_4:

			if(1 == nValue)
			{
				stereoChain1.select(STEREO_CHAIN_MODULE_3);
			}
			else
			{
				ePrevSelect = STEREO_CHAIN_1;
				stereoChain1.deselect();

				eCurrentSelect = MODULE_OUTPUT;
				outModule.select();
			}
			break;

		default:
			break;
		}
		break;

	case STEREO_CHAIN_2:

		switch(stereoChain2.getSelectedModule())
		{
		case STEREO_CHAIN_MODULE_1:

			if(1 == nValue)
			{
				ePrevSelect = STEREO_CHAIN_2;
				stereoChain2.deselect();

				eCurrentSelect = MODULE_INPUT;
				inModule.select();
			}
			else
			{
				stereoChain2.select(STEREO_CHAIN_MODULE_2);
			}
			break;

		case STEREO_CHAIN_MODULE_2:

			if(1 == nValue)
			{
				stereoChain2.select(STEREO_CHAIN_MODULE_1);
			}
			else
			{
				stereoChain2.select(STEREO_CHAIN_MODULE_3);
			}
			break;

		case STEREO_CHAIN_MODULE_3:

			if(1 == nValue)
			{
				stereoChain2.select(STEREO_CHAIN_MODULE_2);
			}
			else
			{
				stereoChain2.select(STEREO_CHAIN_MODULE_4);
			}
			break;

		case STEREO_CHAIN_MODULE_4:

			if(1 == nValue)
			{
				stereoChain2.select(STEREO_CHAIN_MODULE_3);
			}
			else
			{
				ePrevSelect = STEREO_CHAIN_2;
				stereoChain2.deselect();

				eCurrentSelect = MODULE_OUTPUT;
				outModule.select();
			}
			break;

		default:
			break;
		}
		break;

	case STOMP_BOARD:

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

void SystemView::btnUpUpdate()
{
	switch(eCurrentSelect)
	{
	case MONO_CHAIN_2:

		ePrevSelect = MONO_CHAIN_2;
		monoChain2.deselect();

		eCurrentSelect = MONO_CHAIN_1;
		monoChain1.select(monoChain2.getSelectedModule());

		break;

	case MONO_CHAIN_3:

		if(FALSE == inputType.bIsStereo1)
		{
			ePrevSelect = MONO_CHAIN_3;
			monoChain3.deselect();

			eCurrentSelect = MONO_CHAIN_2;
			monoChain2.select(monoChain3.getSelectedModule());
		}
		else
		{
			ePrevSelect = MONO_CHAIN_3;
			monoChain3.deselect();

			eCurrentSelect = STEREO_CHAIN_1;
			stereoChain1.select((StereoChainModules)monoChain3.getSelectedModule());
		}

		break;

	case MONO_CHAIN_4:

		ePrevSelect = MONO_CHAIN_4;
		monoChain4.deselect();

		eCurrentSelect = MONO_CHAIN_3;
		monoChain3.select(monoChain4.getSelectedModule());

		break;

	case STEREO_CHAIN_2:

		if(FALSE == inputType.bIsStereo1)
		{
			ePrevSelect = STEREO_CHAIN_2;
			stereoChain2.deselect();

			eCurrentSelect = MONO_CHAIN_2;
			monoChain2.select((MonoChainModules) stereoChain2.getSelectedModule());
		}
		else
		{
			ePrevSelect = STEREO_CHAIN_2;
			stereoChain2.deselect();

			eCurrentSelect = STEREO_CHAIN_1;
			stereoChain1.select(stereoChain2.getSelectedModule());
		}

		break;

	case STOMP_BOARD:

		ePrevSelect = STOMP_BOARD;
		stompBoard.deselect();

		switch(stompBoard.getSelectedFootSwitch())
		{
		case FOOT_SWITCH_1:

			if(MODULE_OUTPUT == ePrevSelect)
			{
				eCurrentSelect = MODULE_OUTPUT;
				outModule.select();
			}
			else
			{
				if(FALSE == inputType.bIsStereo1)
				{
					eCurrentSelect = MONO_CHAIN_4;
					monoChain4.select(MONO_CHAIN_MODULE_4);
				}
				else
				{
					eCurrentSelect = STEREO_CHAIN_2;
					stereoChain2.select(STEREO_CHAIN_MODULE_4);
				}
			}

			break;
		case FOOT_SWITCH_2:

			if(FALSE == inputType.bIsStereo1)
			{
				eCurrentSelect = MONO_CHAIN_4;
				monoChain4.select(MONO_CHAIN_MODULE_3);
			}
			else
			{
				eCurrentSelect = STEREO_CHAIN_2;
				stereoChain2.select(STEREO_CHAIN_MODULE_3);
			}

			break;
		case FOOT_SWITCH_3:

			if(MODULE_INPUT == ePrevSelect)
			{
				eCurrentSelect = MODULE_INPUT;
				inModule.select();
			}
			else
			{
				if(FALSE == inputType.bIsStereo1)
				{
					eCurrentSelect = MONO_CHAIN_4;
					monoChain4.select(MONO_CHAIN_MODULE_1);
				}
				else
				{
					eCurrentSelect = STEREO_CHAIN_2;
					stereoChain2.select(STEREO_CHAIN_MODULE_1);
				}
			}

			break;
		}

		break;

	case MODULE_INPUT:
	case MODULE_OUTPUT:
	case MONO_CHAIN_1:
	case STEREO_CHAIN_1:
	default:
		break;
	}
}

void SystemView::btnDownUpdate()
{
	switch(eCurrentSelect)
	{
	case MODULE_INPUT:

		ePrevSelect = MODULE_INPUT;
		inModule.deselect();

		eCurrentSelect = STOMP_BOARD;
		stompBoard.select(FOOT_SWITCH_3);

		break;

	case MODULE_OUTPUT:

		ePrevSelect = MODULE_OUTPUT;
		outModule.deselect();

		eCurrentSelect = STOMP_BOARD;
		stompBoard.select(FOOT_SWITCH_1);

		break;

	case MONO_CHAIN_1:

		ePrevSelect = MONO_CHAIN_1;
		monoChain1.deselect();

		eCurrentSelect = MONO_CHAIN_2;
		monoChain2.select(monoChain1.getSelectedModule());

		break;

	case MONO_CHAIN_2:

		if(FALSE == inputType.bIsStereo2)
		{
			ePrevSelect = MONO_CHAIN_2;
			monoChain2.deselect();

			eCurrentSelect = MONO_CHAIN_3;
			monoChain3.select(monoChain2.getSelectedModule());
		}
		else
		{
			ePrevSelect = MONO_CHAIN_2;
			monoChain2.deselect();

			eCurrentSelect = STEREO_CHAIN_2;
			stereoChain2.select((StereoChainModules) monoChain2.getSelectedModule());
		}

		break;

	case MONO_CHAIN_3:

		ePrevSelect = MONO_CHAIN_3;
		monoChain3.deselect();

		eCurrentSelect = MONO_CHAIN_4;
		monoChain4.select(monoChain3.getSelectedModule());

		break;

	case MONO_CHAIN_4:

		eCurrentSelect = STOMP_BOARD;

		switch(monoChain4.getSelectedModule())
		{
		case MONO_CHAIN_MODULE_1:
			stompBoard.select(FOOT_SWITCH_3);
			break;

		case MONO_CHAIN_MODULE_2:
		case MONO_CHAIN_MODULE_3:
			stompBoard.select(FOOT_SWITCH_2);
			break;

		case MONO_CHAIN_MODULE_4:
			stompBoard.select(FOOT_SWITCH_1);
			break;
		default:
			break;
		}

		ePrevSelect = MONO_CHAIN_4;
		monoChain4.deselect();

		break;

	case STEREO_CHAIN_1:

		if(FALSE == inputType.bIsStereo2)
		{
			ePrevSelect = STEREO_CHAIN_1;
			stereoChain1.deselect();

			eCurrentSelect = MONO_CHAIN_3;
			monoChain3.select((MonoChainModules) stereoChain1.getSelectedModule());
		}
		else
		{
			ePrevSelect = STEREO_CHAIN_1;
			stereoChain1.deselect();

			eCurrentSelect = STEREO_CHAIN_2;
			stereoChain2.select(stereoChain1.getSelectedModule());
		}

		break;

	case STEREO_CHAIN_2:

		eCurrentSelect = STOMP_BOARD;

		switch(stereoChain2.getSelectedModule())
		{
		case STEREO_CHAIN_MODULE_1:
			stompBoard.select(FOOT_SWITCH_1);
			break;

		case STEREO_CHAIN_MODULE_2:
		case STEREO_CHAIN_MODULE_3:
			stompBoard.select(FOOT_SWITCH_2);
			break;

		case STEREO_CHAIN_MODULE_4:
			stompBoard.select(FOOT_SWITCH_3);
			break;
		default:
			break;
		}

		ePrevSelect = STEREO_CHAIN_2;
		stereoChain2.deselect();

		break;

	case STOMP_BOARD:
		break;
	}
}

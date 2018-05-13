/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

DeactiveOverlay::DeactiveOverlay() :
	currentState(0)
{
	addAndMakeVisible(descriptionLabel = new Label());

	descriptionLabel->setFont(GLOBAL_BOLD_FONT());
	descriptionLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	descriptionLabel->setEditable(false, false, false);
	descriptionLabel->setJustificationType(Justification::centredTop);

#if USE_TURBO_ACTIVATE
	addAndMakeVisible(resolveLicenseButton = new TextButton("Register Product Key"));
	addAndMakeVisible(registerProductButton = new TextButton("Request offline activation file"));
	addAndMakeVisible(useActivationResponseButton = new TextButton("Activate with activation response file"));
#else
	addAndMakeVisible(resolveLicenseButton = new TextButton("Use License File"));
	addAndMakeVisible(registerProductButton = new TextButton("Activate this computer"));
	addAndMakeVisible(useActivationResponseButton = new TextButton("Activate with activation response file"));
#endif
	addAndMakeVisible(resolveSamplesButton = new TextButton("Choose Sample Folder"));

	addAndMakeVisible(installSampleButton = new TextButton("Install Samples"));

	addAndMakeVisible(ignoreButton = new TextButton("Ignore"));

	resolveLicenseButton->setLookAndFeel(&alaf);
	resolveSamplesButton->setLookAndFeel(&alaf);
	registerProductButton->setLookAndFeel(&alaf);
	useActivationResponseButton->setLookAndFeel(&alaf);
	ignoreButton->setLookAndFeel(&alaf);
	installSampleButton->setLookAndFeel(&alaf);

	resolveLicenseButton->addListener(this);
	resolveSamplesButton->addListener(this);
	registerProductButton->addListener(this);
	useActivationResponseButton->addListener(this);
	ignoreButton->addListener(this);
	installSampleButton->addListener(this);
}

void DeactiveOverlay::buttonClicked(Button *b)
{
	if (b == resolveLicenseButton)
	{
#if USE_COPY_PROTECTION



		Unlocker::resolveLicenseFile(this);

#elif USE_TURBO_ACTIVATE

		const String key = PresetHandler::getCustomName("Product Key", "Enter the product key that you've received along with the download link\nIt should have this format: XXXX-XXXX-XXXX-XXXX-XXXX-XXXX-XXXX");

		TurboActivateUnlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;

		setCustomMessage("Waiting for activation");
		setState(DeactiveOverlay::State::CustomErrorMessage, true);

		ul->activateWithKey(key.getCharPointer());

		setState(DeactiveOverlay::CustomErrorMessage, false);
		setCustomMessage(String());

		setState(DeactiveOverlay::State::CopyProtectionError, !ul->isUnlocked());

		if (ul->isUnlocked())
		{
			PresetHandler::showMessageWindow("Registration successful", "The software is now unlocked and ready to use.");

			FrontendProcessor* fp = dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor());

			fp->updateUnlockedSuspendStatus();

			fp->loadSamplesAfterRegistration();
		}
#endif
	}
	else if (b == installSampleButton)
	{
#if USE_FRONTEND
		auto fpe = findParentComponentOfClass<FrontendProcessorEditor>();

		auto l = new SampleDataImporter(fpe);

		l->setModalBaseWindowComponent(fpe);
#endif
	}
	else if (b == resolveSamplesButton)
	{
		if (currentState[SamplesNotInstalled])
		{
			if (!PresetHandler::showYesNoWindow("Have you installed the samples yet", "Use this only if you have previously installed and extracted all samples from the .hr1 file.\nIf you don't have installed them yet, press cancel to open the sample install dialogue instead"))
			{
#if USE_FRONTEND
				auto fpe = findParentComponentOfClass<FrontendProcessorEditor>();

				auto l = new SampleDataImporter(fpe);

				l->setModalBaseWindowComponent(fpe);
				return;
#endif
			}
		}
		
		FileChooser fc("Select Sample Location", FrontendHandler::getSampleLocationForCompiledPlugin(), "*.*", true);

		if (fc.browseForDirectory())
		{
			FrontendHandler::setSampleLocation(fc.getResult());

			const bool directorySelected = FrontendHandler::getSampleLocationForCompiledPlugin().isDirectory();

			if (directorySelected)
			{
				auto& handler = dynamic_cast<MainController*>(findParentComponentOfClass<AudioProcessorEditor>()->getAudioProcessor())->getSampleManager().getProjectHandler();

#if USE_FRONTEND
				handler.checkAllSampleReferences();

				if (handler.areSampleReferencesCorrect())
				{
					PresetHandler::showMessageWindow("Sample Folder changed", "The sample folder was relocated, but you'll need to open a new instance of this plugin before it can be used.");
				}

				setState(SamplesNotFound, !handler.areSampleReferencesCorrect());
				setState(SamplesNotInstalled, !handler.areSampleReferencesCorrect());
#endif
			}
			else
			{
				setState(SamplesNotFound, true);
			}
		}
	}
	else if (b == registerProductButton)
	{
#if USE_COPY_PROTECTION

		Unlocker::showActivationWindow(this);


#elif USE_TURBO_ACTIVATE

		FileChooser fc("Save activation request file", File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory), "*.xml", true);

		if (fc.browseForFileToSave(true))
		{
			File f = fc.getResult();

#if JUCE_WINDOWS
			TurboActivateCharPointerType path = f.getFullPathName().toUTF16().getAddress();
#else
			TurboActivateCharPointerType path = f.getFullPathName().toUTF8().getAddress();
#endif

			FrontendProcessor* fp = dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor());
			TurboActivateUnlocker* ul = &fp->unlocker;

			ul->writeActivationFile(ul->getProductKey().c_str(), path);

			PresetHandler::showMessageWindow("Activation file created", "Use this file to activate this machine on a computer with internet connection", PresetHandler::IconType::Info);
		}

#endif
	}
	else if (b == useActivationResponseButton)
	{
#if USE_TURBO_ACTIVATE
		FileChooser fc("Load activation response file", File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory), "*.xml", true);

		if (fc.browseForFileToOpen())
		{
			File f = fc.getResult();

#if JUCE_WINDOWS
			TurboActivateCharPointerType path = f.getFullPathName().toUTF16().getAddress();
#else
			TurboActivateCharPointerType path = f.getFullPathName().toUTF8().getAddress();
#endif

			FrontendProcessor* fp = dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor());
			TurboActivateUnlocker* ul = &fp->unlocker;

			setCustomMessage("Waiting for activation");
			setState(DeactiveOverlay::State::CustomErrorMessage, true);

			ul->activateWithFile(path);

			setState(DeactiveOverlay::CustomErrorMessage, false);
			setCustomMessage(String());

			setState(DeactiveOverlay::State::CopyProtectionError, !ul->isUnlocked());

			if (ul->isUnlocked())
			{
				PresetHandler::showMessageWindow("Registration successful", "The software is now unlocked and ready to use.");
				fp->loadSamplesAfterRegistration();
			}

		}
#endif
	}
	else if (b == ignoreButton)
	{
		if (currentState[CustomErrorMessage])
		{
			setState(CustomErrorMessage, false);
		}
		if (currentState[CustomInformation])
		{
			setState(CustomInformation, false);
		}
		else if (currentState[SamplesNotFound])
		{
			auto& handler = dynamic_cast<MainController*>(findParentComponentOfClass<AudioProcessorEditor>()->getAudioProcessor())->getSampleManager().getProjectHandler();

			// Allows partial sample loading the next time
			FRONTEND_ONLY(handler.setAllSampleReferencesCorrect());

			setState(SamplesNotFound, false);
		}
	}
}

#if !USE_COPY_PROTECTION

bool DeactiveOverlay::check(State s, const String &value/*=String()*/)
{
	ignoreUnused(s, value);
	return true;
}

DeactiveOverlay::State DeactiveOverlay::checkLicense(const String &keyContent)
{
	ignoreUnused(keyContent);
	return numReasons;
}

#endif



String DeactiveOverlay::getTextForError(State s) const
{
	switch (s)
	{
	case DeactiveOverlay::AppDataDirectoryNotFound:
		return "The application directory is not found. (The installation seems to be broken. Please reinstall this software.)";
		break;
	case DeactiveOverlay::SamplesNotFound:
		return "The sample directory could not be located. \nClick below to choose the sample folder.";
		break;
	case DeactiveOverlay::SamplesNotInstalled:
		return "Please click below to install the samples from the downloaded archive or point to the location where you've already installed the samples.";
		break;
	case DeactiveOverlay::LicenseNotFound:
	{
#if USE_COPY_PROTECTION
		return "This computer is not registered.\nClick below to authenticate this machine using either online authorization or by loading a license key.";
		break;
#else
		return "";
#endif
	}

	case DeactiveOverlay::ProductNotMatching:
		return "The license key is invalid (wrong plugin name / version).\nClick below to locate the correct license key for this plugin / version";
		break;
	case DeactiveOverlay::MachineNumbersNotMatching:
		return "The machine ID is invalid / not matching.\nClick below to load the correct license key for this computer (or request a new license key for this machine through support.";
		break;
	case DeactiveOverlay::UserNameNotMatching:
		return "The user name is invalid.\nThis means usually a corrupt or rogued license key file. Please contact support to get a new license key.";
		break;
	case DeactiveOverlay::EmailNotMatching:
		return "The email name is invalid.\nThis means usually a corrupt or rogued license key file. Please contact support to get a new license key.";
		break;
	case DeactiveOverlay::CopyProtectionError:
	{
#if USE_TURBO_ACTIVATE
		auto ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;
		return ul->getErrorMessage();
#else
		return "";
#endif
	}
	case DeactiveOverlay::LicenseInvalid:
	{
#if USE_COPY_PROTECTION

		auto ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;

		return ul->getProductErrorMessage();
#else
		return "";
#endif
	}
	case DeactiveOverlay::LicenseExpired:
	{
#if USE_COPY_PROTECTION
		return "The license key is expired. Press OK to reauthenticate (you'll need to be online for this)";
#else
		return "";
#endif
	}
	case DeactiveOverlay::CustomErrorMessage:
		return customMessage;
	case DeactiveOverlay::CriticalCustomErrorMessage:
		return customMessage;
	case DeactiveOverlay::CustomInformation:
		return customMessage;
	case DeactiveOverlay::numReasons:
		break;
	default:
		break;
	}

	return String();
}

void DeactiveOverlay::resized()
{
	useActivationResponseButton->setVisible(false);
	installSampleButton->setVisible(false);
	

	if (currentState != 0)
	{
		descriptionLabel->centreWithSize(getWidth() - 20, 150);
	}

	if (currentState[CustomInformation])
	{
		resolveLicenseButton->setVisible(false);
		registerProductButton->setVisible(false);
		resolveSamplesButton->setVisible(false);
		ignoreButton->setVisible(true);

		ignoreButton->centreWithSize(200, 32);
		ignoreButton->setButtonText("OK");
	}

	if (currentState[CustomErrorMessage])
	{
		resolveLicenseButton->setVisible(false);
		registerProductButton->setVisible(false);
		resolveSamplesButton->setVisible(false);
		ignoreButton->setVisible(true);

		ignoreButton->centreWithSize(200, 32);
		ignoreButton->setButtonText("Ignore");
	}
	

	if (currentState[SamplesNotFound])
	{
		resolveLicenseButton->setVisible(false);
		registerProductButton->setVisible(false);

		

		resolveSamplesButton->setVisible(true);
		ignoreButton->setVisible(true);

		resolveSamplesButton->centreWithSize(200, 32);
		ignoreButton->centreWithSize(200, 32);

		ignoreButton->setTopLeftPosition(ignoreButton->getX(),
			resolveSamplesButton->getY() + 40);

		ignoreButton->setButtonText("Ignore");
	}

	if (currentState[SamplesNotInstalled])
	{
		resolveLicenseButton->setVisible(false);
		registerProductButton->setVisible(false);

		installSampleButton->setVisible(true);

		resolveSamplesButton->setVisible(true);
		ignoreButton->setVisible(false);

		installSampleButton->centreWithSize(200, 32);
		resolveSamplesButton->centreWithSize(200, 32);

		resolveSamplesButton->setTopLeftPosition(resolveSamplesButton->getX(),
			installSampleButton->getY() + 40);
	}

	if (currentState[LicenseNotFound] ||
		currentState[LicenseInvalid] ||
		currentState[MachineNumbersNotMatching] ||
		currentState[UserNameNotMatching] ||
		currentState[ProductNotMatching])
	{
		resolveLicenseButton->setVisible(true);
		registerProductButton->setVisible(true);
		resolveSamplesButton->setVisible(false);
		ignoreButton->setVisible(false);
        installSampleButton->setVisible(false);
        
		resolveLicenseButton->centreWithSize(200, 32);
		registerProductButton->centreWithSize(200, 32);

		resolveLicenseButton->setTopLeftPosition(registerProductButton->getX(),
			registerProductButton->getY() + 40);
	}
	else if (currentState[CopyProtectionError])
	{
		resolveLicenseButton->setVisible(true);

		resolveSamplesButton->setVisible(false);
        installSampleButton->setVisible(false);
		ignoreButton->setVisible(false);

		String text = getTextForError(DeactiveOverlay::State::CopyProtectionError);

		if (text == "Connection to the server failed.")
		{
			registerProductButton->setVisible(true);
			resolveLicenseButton->setVisible(false);
			useActivationResponseButton->setVisible(true);

			registerProductButton->centreWithSize(200, 32);
			useActivationResponseButton->centreWithSize(200, 32);

			useActivationResponseButton->setTopLeftPosition(registerProductButton->getX(), registerProductButton->getY() + 40);

		}
		else
		{
			registerProductButton->setVisible(false);
			resolveLicenseButton->centreWithSize(200, 32);
		}

		if (text.contains("TurboActivate.dat"))
		{
			resolveLicenseButton->setVisible(false);
			registerProductButton->setVisible(false);
		}
	}

	if (currentState[CriticalCustomErrorMessage])
	{
		resolveLicenseButton->setVisible(false);
		registerProductButton->setVisible(false);
		resolveSamplesButton->setVisible(false);
        installSampleButton->setVisible(false);
		ignoreButton->setVisible(false);
	}
}


} // namespace hise
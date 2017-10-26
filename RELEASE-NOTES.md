ID-card utility version [3.12.8](https://github.com/open-eid/qesteidutil/releases/tag/v3.12.8) release notes
--------------------------------------
- Minor fixes and text changes

[Full Changelog](https://github.com/open-eid/qesteidutil/compare/v3.12.7...v3.12.8)

ID-card utility version [3.12.7](https://github.com/open-eid/qesteidutil/releases/tag/v3.12.7) release notes
--------------------------------------
- ECDSA token support
- Remvode M-ID service options
- Minor fixes

[Full Changelog](https://github.com/open-eid/qesteidutil/compare/v3.12.6...v3.12.7)

ID-card utility version [3.12.6](https://github.com/open-eid/qesteidutil/releases/tag/v3.12.6) release notes
--------------------------------------
- Minor fixes

[Full Changelog](https://github.com/open-eid/qesteidutil/compare/v3.12.5...v3.12.6)

ID-card utility version [3.12.5](https://github.com/open-eid/qesteidutil/releases/tag/v3.12.5) release notes
--------------------------------------
- macOS Retina support
- Windows HiDPI support
- OpenSSL 1.1 / LibreSSL compatbility
- Text changes & fixes

[Full Changelog](https://github.com/open-eid/qesteidutil/compare/v3.12.4...v3.12.5)

ID-card utility version 3.12.4 release notes
--------------------------------------
- Show names and other info from the certificates in its original form, do not capitalize
- Added the option for getting diagnostics from command line
- When removing certificates remove all if no card inserted
- Spelling and grammar fixes in all languages
- Support for the next wave of remote certificate updating

ID-card utility version 3.12.3 release notes
--------------------------------------
- Fix parsing certificates longer than 0x0600
- Config option to disable save dialogs
- Removed Qt 4.X support, upstream deprecated
- Make personal and card information copiable in the MainWindow
- Clear tooltips when card is removed
- Fix Digi-ID name visual with long names
- Don't bundle bearer plugins on OSX
- Update E-Mail activation URL
- Handle only cards with known ATR-s (Skip Laptop builtin 3G modems)
- Fix polling when connect returns SCARD_E_NO_SMARTCARD 0x8010000CL


ID-card utility version 3.12.2 release notes
--------------------------------------
- Updater UI changes


ID-card utility version 3.12.1 release notes
--------------------------------------
- Removed smartcardpp usage
- Fixed multiple card support
- Added remote certificate updater for EstEID 3.4 and 3.5 cards
- Update certificate oid detection


ID-card utility version 3.12.0 release notes
--------------------------------------
- Fixed issue with error handling on Ubuntu
- Remove option to update first generation EstEID card certificates
- Cleanup certificates


ID-card utility version 3.11 release notes
--------------------------------------
- Show Chrome Browser Plugin version in diagnostics


ID-card utility version 3.10.1 release notes
--------------------------------------
Changes compared to ver 3.8

- Added "Clean certs" button to "Settings" menu that enables to remove old certificates from Windows Certificate Store.
- Created separate installation package for Windows environment
- Fixed crash that occurred when using RDP session
- Improved visual representation of long e-mail and name values
- Improved representation of the certificates' validity date in the ID-card utility's main window. When the end of validity is at 00:00 then the date of the following day is shown (instead of the previous day). 
- Development of the software can now be monitored in GitHub environment: https://github.com/open-eid/qesteidutil



ID-card utility version 3.8 release notes
--------------------------------------
- Changed the distribution of DigiDoc Utility program for OSX platform. DigiDoc Utility program and DigiDoc3 Client (common application for Client and Crypto) are now available only from Apple App Store. When using ID-card or both ID-card and Mobile-ID then the user is prompted to install an additional software package. 
- Added warning message which is displayed if the inserted ID-card is issued in 2011 and the card’s electronic component has not been renewed. See http://www.politsei.ee/en/teenused/isikut-toendavad-dokumendid/id-kaardi-uuendamine.dot for additional information.
- Fixed problems with Apple App Store version of ID-card utility, including crash report system.
- Changed the default format in which the card owner’s certificates are saved into the user’s computer, PEM format is now used. 
- Changed the public key size calculation method to show the exact size of the key in a certificate’s “public key” field.
- Fixed the problem that did not enable manual updating of the ID-card software in the Settings window before the automatic software updater’s execution is on.
- Added “Run Task Scheduler” button to the Settings window to see the schedule when the next ID-card software update is executed.
- Started using coverity.com static analysis tool to find source code defects and vulnerabilities. Fixed the defects that were discovered.
- Qt framework has been updated to version 5.2 in OSX, version 5.1.1 in Windows environments, version 5.0.2 in Ubuntu 13.10, version 4.8 in Ubuntu 12.04.

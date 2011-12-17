/*	ChipPIO8255.cpp: Class for emulation of PIO 8255 chip
	Copyright (c) 2006 Roman Borik <pmd85emu@gmail.com>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
//---------------------------------------------------------------------------
#include "ChipPIO8255.h"
/**
 * Konstruktor pre vytvorenie objektu cipu PIO 8255. Zodpoveda stavu Power-up,
 * teda pripojeniu napajania.
 * Na rozdiel od originalneho cipu, ktory je po pripojeni napajania v neurcitom
 * stave, je mozne v konstruktore previest ihned reset cipu.
 * Zaroven sa zrusia vsetky notifykacne funkcie.

 * @param reset ak je true, prevedie sa zaroven reset cipu
 */
ChipPIO8255::ChipPIO8255(bool reset)
{
	if (reset == true)
		ChipReset(true);
	else
		ClearAllNotifyFunctions();
}
//---------------------------------------------------------------------------
/** Privatna metoda pre zrusenie vsetkych notifykacnych funkcii. */
void ChipPIO8255::ClearAllNotifyFunctions()
{
	OnCpuWriteA.disconnect_all();
	OnCpuWriteB.disconnect_all();
	OnCpuWriteC.disconnect_all();
	OnCpuWriteCH.disconnect_all();
	OnCpuWriteCL.disconnect_all();
	OnCpuWriteCWR.disconnect_all();
	OnCpuReadA.disconnect_all();
	OnCpuReadB.disconnect_all();
	OnCpuReadC.disconnect_all();
	OnCpuReadCH.disconnect_all();
	OnCpuReadCL.disconnect_all();
	OnCpuReadCWR.disconnect_all();
}
//---------------------------------------------------------------------------
/**
 * Metoda ChipReset prevedie reset cipu. Zodpoveda privedeniu urovne H na vstup
 * RESET (35).
 * Volitelne je mozne zrusit vsetky notifikacne funkcie.
 *
 * @param clearNotifyFunc ak je true, zrusi vsetky notifykacne funkcie
 */
void ChipPIO8255::ChipReset(bool clearNotifyFunc)
{
	if (clearNotifyFunc == true)
		ClearAllNotifyFunctions();

	// nastav vsetky porty do modu 0
	// vynuluj vsetky vnutorne registre
	CpuWrite(PP_CWR, BASIC_CWR); // b10011011
	InLatchA = 0; // samotne nastavenie rezimu nenuluje vstupny latch
	InLatchB = 0;
}
//---------------------------------------------------------------------------
/**
 * Metoda GetChipState je pouzivana pri vytvarani Snapshotu a ulozi niektore
 * vnutorne registre chipu do buffra. Ak je buffer null, vrati potrebnu velkost
 * buffra v bytoch.
 * Data sa do buffra ulozia v poradi: CWR, PC, PB, PA, prerusenia
 *
 * @param buffer buffer kam sa ulozi stav chipu
 * @return pocet bytov ulozenych do buffra
 */
int ChipPIO8255::GetChipState(BYTE *buffer)
{
	if (buffer != NULL) {
		*buffer = CWR;
		*(buffer + 1) = OutLatchC;
		*(buffer + 2) = OutLatchB;
		*(buffer + 3) = OutLatchA;
		*(buffer + 4) = (BYTE) (
			((InteAin == true) ? 1 : 0) |
			((InteAout == true) ? 2 : 0) |
			((InteB == true) ? 4 : 0)
		);
	}

	return 5;
}
//---------------------------------------------------------------------------
/**
 * Metoda SetChipState je pouzivana po otvoreni Snapshotu pre prednastavenie
 * niektorych vnutornych registrov chipu.
 * Data v buffri musia byt v poradi: CWR, PC, PB, PA, prerusenia
 *
 * @param buffer buffer kam sa ulozi stav chipu
 * @return pocet bytov ulozenych do buffra
 */
void ChipPIO8255::SetChipState(BYTE *buffer)
{
	if (buffer != NULL && *buffer != 0) {
		ChipReset(false);
		CpuWrite(PP_CWR, *buffer);
		CpuWrite(PP_PortC, *(buffer + 1));
		CpuWrite(PP_PortB, *(buffer + 2));
		CpuWrite(PP_PortA, *(buffer + 3));
		if (*(buffer + 4) & 1)
			CpuWrite(PP_CWR, 9);  // InteAin - PC4 <- 1
		if (*(buffer + 4) & 2)
			CpuWrite(PP_CWR, 13); // InteAout - PC6 <- 1
		if (*(buffer + 4) & 4)
			CpuWrite(PP_CWR, 5);  // InteB - PC2 <- 1
	}
}
//---------------------------------------------------------------------------
/**
 * Privatna metoda NotifyOnWritePortC pozivana pri zapise na port C procesorom.
 * K notifikacii dojde iba v pripade, ze sa vystupna hodnota na porte C zmenila.
 * POZOR: Notifikacia je prevedena bud volanim funkcie OnCpuWriteC alebo dvojice
 * OnCpuWriteCH a OnCpuWriteCL. OnCpuWriteC ma vyssiu prioritu. Teda, ak je
 * nastavena adresa funkcie OnCpuWriteC, notifikacne funkcie pre polovice portu
 * su ignorovane.
 *
 * @param oldVal povodna hodnota na vystupe portu C
 * @param newVal nova hodnota posielana na vystup portu C
 */
void ChipPIO8255::NotifyOnWritePortC(BYTE oldVal, BYTE newVal)
{
	BYTE val;

	val = oldVal ^ newVal;
	if (val && OnCpuWriteC.isset())
		OnCpuWriteC();
	else {
		if ((val & 0xF0) && OnCpuWriteCH.isset())
			OnCpuWriteCH();
		if ((val & 0x0F) && OnCpuWriteCL.isset())
			OnCpuWriteCL();
	}
}
//---------------------------------------------------------------------------
/**
 * Metodu CpuWrite vola mikroprocesor pri vykonavani instrukcie OUT - zapis na
 * port (CPU -> PIO). Zodpoveda teda privedeniu urovne L na vstupy /CS (6) a
 * /WR (36).
 * PO zapisani hodnoty 'val' na cielovy port 'dest' zavola prislusnu
 * notifikacnu funkciu (ak bola konkretnou periferiou nastavena), aby mohla
 * periferia data spracovat. Pri portoch A a B sa to tyka, iba ak su v Mode 0.
 * V Modoch 1 a 2 je periferia notifikovana notifikacnymi funkciami portu C
 * prostrednictvom handshake signalov.
 *
 * @param dest oznacuje port (TPIOPort), na ktory je posielana hodnota 'val'
 * @param val hodnota posielana na dany port
 */
void ChipPIO8255::CpuWrite(TPIOPort dest, BYTE val)
{
	BYTE oldVal;
	BYTE mode;

	switch (dest) {
		case PP_PortA :
			if ((CWR & GA_MODE) == GA_MODE0) { // Mod 0
				// zmena hodnoty
				OutLatchA = val;
				OnCpuWriteA();  // notifikacia
			}
			else { // Mod 1, 2
				OutLatchA = val;
				oldVal = OutLatchC;
				OutLatchC &= (BYTE)(~(INTRA_MASK | _OBFA_MASK));
				// notifikacia
				NotifyOnWritePortC(oldVal, OutLatchC);
			}
			break;

		case PP_PortB :
			if ((CWR & GB_MODE) == GB_MODE0) { // Mod 0
				// zmena hodnoty
				OutLatchB = val;
				OnCpuWriteB();  // notifikacia
			}
			else { // Mod 1
				OutLatchB = val;
				oldVal = OutLatchC;
				OutLatchC &= ~(INTRB_MASK | _OBFB_MASK);
				// notifikacia
				NotifyOnWritePortC(oldVal, OutLatchC);
			}
			break;

		case PP_PortC :
			// ovplyvnene su iba vystupne bity v Mode 0
			oldVal = OutLatchC;

			if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT)
					&& (CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT))
				OutLatchC = val;
			else if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT))
				OutLatchC = (BYTE)((OutLatchC & 0xF0) | (val & 0x0F));
			else if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT))
				OutLatchC = (BYTE)((OutLatchC & 0x0F) | (val & 0xF0));

			// notifikacia
			NotifyOnWritePortC(oldVal, OutLatchC);
			break;

		case PP_CWR :
			if (val & CWR_MASK) {
				// nastavenie rezimu PIO
				CWR = val;
				if ((CWR & GA_MODE) == GA_MODE)
					CWR &= ~GA_MODE1;

				// vynuluj vsetky vnutorne registre
				InBufferA = 0xFF; // Pull-up
//				InLatchA = 0; nastavenie rezimu nenuluje vstupny latch
				OutLatchA = 0;

				InBufferB = 0xFF; // Pull-up
				if ((CWR & GB_MODE) != GB_MODE0)
					InLatchB = 0;
				OutLatchB = 0;

				InBufferC = 0;
				if ((CWR & GB_MODE) == GB_MODE0)
					InBufferC |= 0x07; // Pull-up
				if ((CWR & GA_MODE) == GA_MODE0)
					InBufferC |= 0xF8;

//				oldVal = OutLatchC;
				OutLatchC = 0;
				if ((CWR & GB_MODE) != GB_MODE0)
					OutLatchC |= _OBFB_MASK;
				if ((CWR & GA_MODE) != GA_MODE0)
					OutLatchC |= _OBFA_MASK;

				// zakaz preruseni
				InteAin = false;
				InteAout = false;
				InteB = false;

				OnCpuWriteCWR(CWR);
/*
				if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT)
						&& (CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT))
					oldVal = ~OutLatchC;
				else {
					if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT))
						oldVal = OutLatchC ^ 0x07;
					if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT))
						oldVal = OutLatchC ^ 0xF8;
				}
				NotifyOnWritePortC(oldVal, OutLatchC);
*/
				NotifyOnWritePortC((BYTE)(~OutLatchC), OutLatchC);
			}
			else {
				// nastavenie bitov portu C
				bool inte = false;
				val &= 0x0F;

				mode = (BYTE)(CWR & (GA_MODE | PORTA_DIR));
				oldVal = OutLatchC;

				if ((mode == (GA_MODE1 | PORTA_INP) || (CWR & GA_MODE) == GA_MODE2) && (val & 0x0E) == 8) {
					InteAin = (val & 1);
					inte = true;
//					debug("InteAin=%d", InteAin);
				}
				else if ((mode == (GA_MODE1 | PORTA_OUT) || (CWR & GA_MODE) == GA_MODE2) && (val & 0x0E) == 12) {
					InteAout = (val & 1);
					inte = true;
//					debug("InteAout=%d", InteAout);
				}

				if (inte == true) {
					OutLatchC &= ~INTRA_MASK;
					if ((InteAin == true && (InBufferC & _STBA_MASK) == _STBA_MASK
							 && (OutLatchC & IBFA_MASK) == IBFA_MASK)
							|| (InteAout == true && (InBufferC & _ACKA_MASK) == _ACKA_MASK
									&& (OutLatchC & _OBFA_MASK) == _OBFA_MASK)) {
						OutLatchC |= INTRA_MASK;
					}
//					debug("OutLatchC=%u, InBufferC=%u", OutLatchC, InBufferC);
				}

				if ((CWR & GB_MODE) == GB_MODE1 && (val & 0x0E) == 4) {
					InteB = (val & 1);
					OutLatchC &= ~INTRB_MASK;
					if (InteB == true) {
						if (((CWR & PORTB_INP) == PORTB_INP
								 && (InBufferC & _STBB_MASK) == _STBB_MASK
								 && (OutLatchC & IBFB_MASK) == IBFB_MASK)
								|| ((CWR & PORTB_OUT) == PORTB_OUT
										&& (InBufferC & _ACKB_MASK) == _ACKB_MASK
										&& (OutLatchC & _OBFB_MASK) == _OBFB_MASK)) {
							OutLatchC |= INTRB_MASK;
						}
					}
					inte = true;
				}

				if (inte == false) {
					if (((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT)
						 && (CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT))
						|| ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT)
								&& (CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_INP)
								&& (val >> 1) < 4)
						|| ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_INP)
								&& (CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT)
								&& (val >> 1) > 3)
						|| ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT)
								&& (val >> 1) < 3)
						|| ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT)
								&& (val >> 1) > 2))
					{
						if (val & 1)
							OutLatchC |= (BYTE)((1 << (val >> 1)));
						else
							OutLatchC &= (BYTE)(~(1 << (val >> 1)));
					}
				}

				NotifyOnWritePortC(oldVal, OutLatchC);
			}
			break;

		default :
			warning("ChipPIO8255::CpuWrite > invalid PIO port: %d", dest);
			break;
	}
}
//---------------------------------------------------------------------------
/**
 * Metodu CpuRead vola mikroprocesor pri vykonavani instrukcie IN - citanie z
 * portu (CPU <- PIO). Zodpoveda teda privedeniu urovne L na vstupy /CS (6) a
 * /RD (5).
 * PRED precitanim hodnoty zo zdrojoveho portu 'src' zavola prislusnu
 * notifikacnu funkciu (ak bola konkretnou periferiou nastavena), aby mohla
 * periferia potrebne data pripravit. Pri portoch A a B sa to tyka, iba ak su
 * v Mode 0. V Modoch 1 a 2 je periferia notifikovana notifikacnymi funkciami
 * portu C prostrednictvom handshake signalov.
 *
 * @param src oznacuje port (TPIOPort), z ktoreho sa ma udaj precitat
 * @return hodnota precitana z portu
 */
BYTE ChipPIO8255::CpuRead(TPIOPort src)
{
	BYTE ret_val = 0;
	BYTE oldVal;
	BYTE mode;

	switch (src) {
		case PP_PortA :
			mode = (BYTE)(CWR & (GA_MODE | PORTA_DIR));

			// notifikacia je len v mode 0, vstup
			// v ostatnych modoch sa uplatnuju handshake signaly
			if (mode == (GA_MODE0 | PORTA_INP))
				OnCpuReadA();

			if (mode == (GA_MODE0 | PORTA_INP))  // Mod 0, Vstup
				ret_val = InBufferA;  // precitanie hodnoty zo vstupu
			else if (mode == (GA_MODE0 | PORTA_OUT))  // Mod 0, Vystup
			// precitanie hodnoty vystupneho latchu s ohladom na stav vstupnych liniek
				ret_val = OutLatchA & InBufferA;
			else if (mode == (GA_MODE1 | PORTA_OUT))  // Mod 1, Vystup
				ret_val = OutLatchA;  // precitanie hodnoty vystupneho latchu
			else if (mode == (GA_MODE1 | PORTA_INP) // Mod 1, Vstup
							 || (CWR & GA_MODE) == GA_MODE2) { // alebo Mod 2
				ret_val = InLatchA; // precitanie hodnoty, ktoru dodala periferia
				// signal /RD nuluje bity INTRA a IBFA
				oldVal = OutLatchC;
				OutLatchC &= ~(INTRA_MASK | IBFA_MASK);
				NotifyOnWritePortC(oldVal, OutLatchC);
			}
			break;

		case PP_PortB :
			mode = (BYTE)(CWR & (GB_MODE | PORTB_DIR));

			// notifikacia je len v mode 0, vstup
			// v ostatnych modoch sa uplatnuju handshake signaly
			if (mode == (GB_MODE0 | PORTB_INP))
				OnCpuReadB();

			if (mode == (GB_MODE0 | PORTB_OUT)) // Mod 0, Vystup
				ret_val = OutLatchB;  // precitanie hodnoty vystupneho latchu
			else if (mode == (GB_MODE0 | PORTB_INP)) // Mod 0, Vstup
				ret_val = InBufferB;  // precitanie hodnoty zo vstupu
			else if (mode == (GB_MODE1 | PORTB_OUT))  // Mod 1, Vystup
				ret_val = OutLatchB;  // precitanie hodnoty vystupneho latchu
			else if (mode == (GB_MODE1 | PORTB_INP)) {  // Mod 1, Vstup
				ret_val = InLatchB; // precitanie hodnoty, ktoru dodala periferia
				// signal /RD nuluje bity INTRB a IBFB
				oldVal = OutLatchC;
				OutLatchC &= ~(INTRB_MASK | IBFB_MASK);
				NotifyOnWritePortC(oldVal, OutLatchC);
			}
			break;

		case PP_PortC :
			ret_val = 0;

			if (OnCpuReadCH.isset() || OnCpuReadCL.isset()) {
				 // Mod 0, Vstup
				if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_INP))
					OnCpuReadCH();

				if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_INP))
					OnCpuReadCL();
			}
			else if (OnCpuReadC.isset()) {
				 // Mod 0, Vstup
				if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_INP)
				 || (CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_INP))
					OnCpuReadC();
			}

			if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT)) // Mod 0, Vystup
				ret_val |= (BYTE)(OutLatchC & 0x0F);
			else if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_INP)) // Mod 0, Vstup
				ret_val |= (BYTE)(InBufferC & 0x0F);
			if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT)) // Mod 0, Vystup
				ret_val |= (BYTE)(OutLatchC & 0xF0);
			else if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_INP)) // Mod 0, Vstup
				ret_val |= (BYTE)(InBufferC & 0xF0);

			switch (CWR & GA_MODE) {
				case GA_MODE1 :
					if ((CWR & PORTA_DIR) == PORTA_INP) {
						ret_val &= 0xC7;
						if (InteAin == true)
							ret_val |= INTEAIN_MASK;
						ret_val |= (BYTE)(OutLatchC & IBFA_MASK);
					}
					else {
						ret_val &= 0x37;
						if (InteAout == true)
							ret_val |= INTEAOUT_MASK;
						ret_val |= (BYTE)(OutLatchC & _OBFA_MASK);
					}
					ret_val |= (BYTE)(OutLatchC & INTRA_MASK);
					break;

				case GA_MODE2 :
					ret_val &= 0x07;
					if (InteAin == true)
						ret_val |= INTEAIN_MASK;
					ret_val |= (BYTE)(OutLatchC & IBFA_MASK);
					if (InteAout == true)
						ret_val |= INTEAOUT_MASK;
					ret_val |= (BYTE)(OutLatchC & _OBFA_MASK);
					ret_val |= (BYTE)(OutLatchC & INTRA_MASK);
					break;
			}

			if ((CWR & GB_MODE) == GB_MODE1) {
				ret_val &= 0xF8;
				if (InteB == true)
					ret_val |= INTEB_MASK;
				ret_val |= (BYTE)(OutLatchC & INTRB_MASK);
				if ((CWR & PORTB_DIR) == PORTB_INP)
					ret_val |= (BYTE)(OutLatchC & IBFB_MASK);
				else
					ret_val |= (BYTE)(OutLatchC & _OBFB_MASK);
				if ((CWR & GA_MODE) == GA_MODE0) {
					ret_val &= 0xF7;
					if ((CWR & PORTCH_DIR) == PORTCH_INP)
						ret_val |= (BYTE)(InBufferC & 0x08);
					else
						ret_val |= (BYTE)(OutLatchC & 0x08);
				}
			}
			break;

		case PP_CWR :     // NMOS verzia 8255 neumoznuje citanie CWR
			ret_val = CWR;  // toto je mozne len v CMOS verzii 82C55
			break;

		default :
			warning("ChipPIO8255::CpuRead > invalid PIO port: %d", src);
			ret_val = 0;
			break;
	}

	return ret_val;
}
//---------------------------------------------------------------------------
void ChipPIO8255::PeripheralWriteByte(TPIOPort dest, BYTE val)
{
	BYTE oldVal;

	switch (dest) {
		case PP_PortA :
			InBufferA = val;
			break;

		case PP_PortB :
			InBufferB = val;
			break;

		case PP_PortC :
			oldVal = OutLatchC;
			if ((CWR & GB_MODE) == GB_MODE1) {
				// vstup
				if ((CWR & PORTB_DIR) == PORTB_INP) {
					if ((InBufferC & _STBB_MASK) == _STBB_MASK && (val & _STBB_MASK) == 0) {
						// /STB  --__
						InLatchB = InBufferB;
						OutLatchC |= IBFB_MASK;
						OutLatchC &= ~INTRB_MASK;
					}
					else if ((InBufferC & _STBB_MASK) == 0 && (val & _STBB_MASK) == _STBB_MASK) {
						// /STB  __--
						if (InteB == true && (OutLatchC & IBFB_MASK) == IBFB_MASK)
							OutLatchC |= INTRB_MASK;
						else
							OutLatchC &= ~INTRB_MASK;
					}
				}
				else { // vystup
					if ((InBufferC & _ACKB_MASK) == _ACKB_MASK && (val & _ACKB_MASK) == 0) {
						// /ACK  --__
						OutLatchC |= _OBFB_MASK;
						OutLatchC &= ~INTRB_MASK;
					}
					else if ((InBufferC & _ACKB_MASK) == 0 && (val & _ACKB_MASK) == _ACKB_MASK) {
						// /ACK  __--
						if (InteB == true && (OutLatchC & _OBFB_MASK) == _OBFB_MASK)
							OutLatchC |= INTRB_MASK;
						else
							OutLatchC &= ~INTRB_MASK;
					}
				}
			}

			if ((CWR & GA_MODE) != GA_MODE0) {
				// vstup
				if ((CWR & GA_MODE) == GA_MODE2
						|| (CWR & (GA_MODE | PORTA_DIR)) == (GA_MODE1 | PORTA_INP)) {
					if ((InBufferC & _STBA_MASK) == _STBA_MASK && (val & _STBA_MASK) == 0) {
						// /STB  --__
						InLatchA = InBufferA;
						OutLatchC |= IBFA_MASK;
						OutLatchC &= ~INTRA_MASK;
					}
					else if ((InBufferC & _STBA_MASK) == 0 && (val & _STBA_MASK) == _STBA_MASK) {
						// /STB  __--
						if (InteAin == true && (OutLatchC & IBFA_MASK) == IBFA_MASK)
							OutLatchC |= INTRA_MASK;
						else
							OutLatchC &= ~INTRA_MASK;
					}
				}

				// vystup
				if ((CWR & GA_MODE) == GA_MODE2
						|| (CWR & (GA_MODE | PORTA_DIR)) == (GA_MODE1 | PORTA_OUT)) {
					if ((InBufferC & _ACKA_MASK) == _ACKA_MASK && (val & _ACKA_MASK) == 0) {
						// /ACK  --__
						OutLatchC |= _OBFA_MASK;
						OutLatchC &= ~INTRA_MASK;
					}
					else if ((InBufferC & _ACKA_MASK) == 0 && (val & _ACKA_MASK) == _ACKA_MASK) {
						// /ACK  __--
						if (InteAout == true && (OutLatchC & _OBFA_MASK) == _OBFA_MASK)
							OutLatchC |= INTRA_MASK;
						else
							OutLatchC &= ~INTRA_MASK;
					}
				}
			}

			InBufferC = val;
			NotifyOnWritePortC(oldVal, OutLatchC);
			break;

		default :
			warning("ChipPIO8255::PeripheralWriteByte > invalid PIO port: %d", dest);
			break;
	}
}
//---------------------------------------------------------------------------
void ChipPIO8255::PeripheralChangeBit(TPIOPort dest, TPIOPortBit bit, bool state)
{
	BYTE val;

	switch (dest) {
		case PP_PortA :
			InBufferA &= (BYTE)(~(1 << bit));
			InBufferA |= (BYTE)((state == true) ? (1 << bit) : 0);
			break;

		case PP_PortB :
			InBufferB &= (BYTE)(~(1 << bit));
			InBufferB |= (BYTE)((state == true) ? (1 << bit) : 0);
			break;

		case PP_PortC :
			if ((CWR & GB_MODE) == GB_MODE1 && (bit == _STBB || bit == _ACKB)) {
				if (bit == _STBB)
					val = (BYTE)((InBufferC & ~_STBB_MASK) | ((state == true) ? _STBB_MASK : 0));
				else
					val = (BYTE)((InBufferC & ~_ACKB_MASK) | ((state == true) ? _ACKB_MASK : 0));
				PeripheralWriteByte(dest, val);
			}
			else if ((CWR & GA_MODE) != GA_MODE0 && (bit == _STBA || bit == _ACKA)) {
				if (bit == _STBA)
					val = (BYTE)((InBufferC & ~_STBA_MASK) | ((state == true) ? _STBA_MASK : 0));
				else
					val = (BYTE)((InBufferC & ~_ACKA_MASK) | ((state == true) ? _ACKA_MASK : 0));
				PeripheralWriteByte(dest, val);
			}
			else {
				InBufferC &= (BYTE)(~(1 << bit));
				InBufferC |= (BYTE)((state == true) ? (1 << bit) : 0);
			}
			break;

		default :
			warning("ChipPIO8255::PeripheralChangeBit > invalid PIO port: %d", dest);
			break;
	}
}
//---------------------------------------------------------------------------
BYTE ChipPIO8255::PeripheralReadByte(TPIOPort src)
{
	BYTE ret;

	switch (src) {
		case PP_PortA :
			if ((CWR & GA_MODE) == GA_MODE2) {
				if ((InBufferC & _ACKA_MASK) == 0)
					ret = OutLatchA;
				else if ((InBufferC & _STBA_MASK) == 0)
					ret = InBufferA;
				else
					ret = 0xFF;
			}
			else {
				if ((CWR & PORTA_DIR) == PORTA_OUT)
					ret = OutLatchA;
				else
					ret = InBufferA;
			}
			break;

		case PP_PortB :
			if ((CWR & PORTB_DIR) == PORTB_OUT)
				ret = OutLatchB;
			else
				ret = InBufferB;
			break;

		case PP_PortC :
			ret = 0;
			if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT))
				ret |= (BYTE)(OutLatchC & 0x07);
			else if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_INP))
				ret |= (BYTE)(InBufferC & 0x07);
			else if ((CWR & GB_MODE) == GB_MODE1) {
				ret |= (BYTE)(OutLatchC & 0x03);
				ret |= (BYTE)(InBufferC & 0x04);
			}

			if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT))
				ret |= (BYTE)(OutLatchC & 0xF0);
			else if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_INP))
				ret |= (BYTE)(InBufferC & 0xF0);
			else if ((CWR & (GA_MODE | PORTA_DIR)) == (GA_MODE1 | PORTA_OUT)) {
				ret |= (BYTE)(OutLatchC & 0x88);
				ret |= (BYTE)(InBufferC & 0x70);
			}
			else if ((CWR & (GA_MODE | PORTA_DIR)) == (GA_MODE1 | PORTA_INP)) {
				ret |= (BYTE)(OutLatchC & 0x28);
				ret |= (BYTE)(InBufferC & 0xD0);
			}
			else if ((CWR & GA_MODE) == GA_MODE2) {
				ret |= (BYTE)(OutLatchC & 0xA8);
				ret |= (BYTE)(InBufferC & 0x50);
			}

			if ((CWR & GA_MODE) == GA_MODE0) {
				if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT))
					ret |= (BYTE)(OutLatchC & 0x08);
				else if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_INP))
					ret |= (BYTE)(InBufferC & 0x08);
			}
			break;

		default :
			warning("ChipPIO8255::PeripheralReadByte > invalid PIO port: %d", src);
			ret = 0xFF;
			break;
	}

	return ret;
}
//---------------------------------------------------------------------------
bool ChipPIO8255::PeripheralReadBit(TPIOPort src, TPIOPortBit bit)
{
	bool ret;

	switch (src) {
		case PP_PortA :
			ret = PeripheralReadByte(PP_PortA) & (1 << bit);
			break;

		case PP_PortB :
			ret = PeripheralReadByte(PP_PortB) & (1 << bit);
			break;

		case PP_PortC :
			ret = PeripheralReadByte(PP_PortC) & (1 << bit);
			break;

		default :
			warning("ChipPIO8255::PeripheralReadBit > invalid PIO port: %d", src);
			ret = false;
			break;
	}

	return ret;
}
//---------------------------------------------------------------------------

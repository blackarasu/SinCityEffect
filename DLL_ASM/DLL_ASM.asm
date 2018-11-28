;***
;DLL_ASM.asm - Projekt JA Sin City Effect
;
;	Data utworzenia: 17-10-2018 17:31
;  
;Cel:
;       Celem projektu jest wykorzystanie wektorowych operacji SIND,
;		i u¿ywanie instrukcji SSE, zastosowanie wielow¹tkowoœci.
;		Aplikacja zamienia wczytany obraz zapisany w RGB na HSL, 
;		Sprawdza pixel czy ma okreœlony odcieñ (H) i jeœli
;			TAK - zwiêksza jego saturacjê (S) o pewn¹ wartoœæ
;			NIE - ustawia jego saturacjê na 0
;		i z powrotem zamienia na RGB
;
;
;*******************************************************************************
;S³ownik:
; R8D -> --- - Wartoœæ HUE do porównania
; RCX -> R8 - adres do tablicy Hue
; RDX -> R9 - adres do tablicy Saturations
; R9D -> EAX -> ECX - wartoœæ iloœci elementów do policzenia/ilosc skokow petli
; XMM0 - Saturacje
; XMM1 - INCREASE_S
; EAX/XMM2,XMM3 - HUE(S)
; XMM4/R10D - H-RANGE
; XMM5/R11D - H+RANGE
;------------------------------------------------------------------------- 


OPTION CASEMAP:NONE

.NOLIST 
.NOCREF 

.LIST

.data;miejsce na sta³e
RANGE DWORD 14
INCREASE_S REAL4 0.3
.code;tu pisac procedury

ProcedeS PROC						;Przeksztalc S z HSL 
MOVD XMM4, R8D						;przeniesienie wartosci do porownania
SUBSS XMM4, RANGE					;bedzie sluzyc jako czy wieksze lub równe niz
SHUFPS XMM4, XMM4, 00h				;zrobienie z tego wektora
MOVD XMM5, R8D						;wartosc do porownania
ADDSS XMM5, RANGE					;bedzie sluzyc jako czy mniejsze lub równe niz
SHUFPS XMM5, XMM5, 00h				;zrobienie z tego wektora 
MOVD XMM1, INCREASE_S				;bedzie sluzyc jako zwieksz saturacje
SHUFPS XMM1, XMM1, 00h				;zrobienie wektora
MOV EAX, R9D						;skopiowanie do akumulatora ilosci elementow
MOV EBX, 4h							;przygotowanie do sprawdzenia modulo z 4
MOV R8, RCX							;Zachowanie adresu do tablicy Hues (RCX/ECX bêdzie to counter do main loop)
MOV R9, RDX							;Zachowanie tablicy Saturations
XOR EDX, EDX						;przygotowanie do sprawdzenia modulo part 2
IDIV EBX							;podzielenie przez 4(wartosc ile bierzemy wartosci do xmm)
MOV ECX, EAX						;LOOP counter to ilosc elementow /4
CMP EDX, 0h							;Sprawdzenie czy reszta z dzielenia to 0
JZ RLY_SMALL_DATA					;jesli tak to nie trzeba nic fixowac
MOVD R10D, XMM4						;H-RANGE
MOVD R11D, XMM5						;H+RANGE
MOD4FIX_LOOP: MOV EAX, DWORD PTR[R8];hue. Jeœli nie to fix. Zajete: EDX,RBX,ECX Wykorzystane w: EAX - Hue, XMM0 - Saturation, R8D - +RANGE, R9D - -RANGE
MOVD XMM0, REAL4 PTR [R9]			;saturacja
CMP EAX,R10D						;sprawdza czy HUE z elementu wiêksze od H - RANGE
JL WYZERUJ							;Jesli nie to wyzeruj i idŸ dalej
CMP EAX, R11D						;sprawdza czy HUE z elementu jest mniejszy lub rowny od H + RANGE
JG WYZERUJ							;jezeli 
ADDSS XMM0,XMM1						;zwiekszenie saturacji 
JMP NEXT							;ominiecie wyzerowania
WYZERUJ: XORPS XMM0, XMM0			;wyzerowanie saturacji
NEXT: MOVD REAL4 PTR [R9],XMM0		;Zapisanie wartoœci do pamiêci
ADD R8, 4h							;przejscie do kolejnego elementu tablicy Hues
ADD R9, 4h							;przejscie do kolejnego elementu tablicy Saturations
DEC EDX								;dekrementuj fixloop counter
JNZ MOD4FIX_LOOP					;skok jeœli petla sie nie skoñczy³a
RLY_SMALL_DATA: CMP ECX, 0h			;sprawdzamy czy wprowadzono do procedury bardzo male dane
JZ EXIT								;nie ma nic do wyliczenia (prawdopodobnie maly fix)
MAIN_LOOP: MOVUPS XMM0, [R9]		;przypisanie 4 wartoœci saturacji do XMM0
MOVDQU XMM2, [R8]					;przpisanie 4 wartoœci hue do XMM2
MOVDQU XMM3, XMM2					;kopia, u¿yta do porownania
ADDPS XMM0, XMM1					;zwiekszenie wszystkich saturacji o rz¹dan¹ wartoœæ
CMPLEPS XMM2,XMM5					;Sprawdz czy wartosc jest mniejsza b¹dŸ równa H+Range
ANDPS XMM0, XMM2					;pierwszy warunek (wyzerowanie to co juz nie spelnia)
CMPNLEPS XMM3,XMM4					;Sprawdz czy wartoœæ HUE jest wiêksza b¹dŸ równa H-Range
ANDPS XMM0, XMM3					;drugi warunek (wyzerowanie pozosta³oœci, które nie spa³niaj¹ warunku)
MOVUPS xmmword ptr[R9], XMM0		;przerzucenie 4 wartoœci  saturations do pamiêci
ADD R8, 10h							;przeskok do nastepnych 4 elementów hue
ADD R9, 10h							;przeskok do nastepnych 4 elementów saturations
LOOP MAIN_LOOP						;petla glownej petli
EXIT:ret
ProcedeS endp
END

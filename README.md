# Nawijarka-do-cewek-z-panelem-sterujacym-Weintek
Program odpowiada za sterowanie nawijarką do cewek wraz z podłączonym panelem Weinteka (jest on również odpowiedzialny za komunikację między nimi).
Nawijarka działa na dwóch silnikach korkowych połączonych, których ruch połączony jest metodą interpolacji zaimplementowaną w kodzie.

Funkjce zaimplementowane dla panelu Weintek:
- Podanie ilości zwojów cewki
- Podanie długości cewki
- podanie warstw cewki
- określenie kierunku nawijania cewki
- funkcje stopu i startu procesu nawijania

Funkcje zaimplementowane dla ruchu silników nawijarki:
- sterowanie ruchem silnika odpowiedzialnego za przesuwanie wózka wraz ze szpulą miedzi
- sterowanie ruchem silnika odpowiedzialnego za obracanie wału, na który nawijana jest cewka
- interpolacja dla silnika wału oraz silnika wózka

Pozostałe funkcje:
- funkcja komunikacji między panelem a nawijarką
- funkcje stopu oraz startu procesu
- funkcje inicjalizacyjne

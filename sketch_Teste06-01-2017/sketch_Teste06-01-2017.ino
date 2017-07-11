#include <SPI.h>
#include <String.h>
#include <Ethernet_v2.h>
#include "DHT.h"
//#include <DS1307.h>

//DS1307 rtc(A4, A5);//relogio



//definindo variaveis de rede
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0xE6, 0xAD};//informar o mac q vem no ethernet shild
byte ipClient[] = {192,168,1,102};// 192,168,0,101 ip do arduino 
byte ipServer[] = {192,168,1,101};// 192,168,0,103 ip do servidor
byte gateway[] = { 192,168,1, 1 };  // 192,168,0, 1 ip do roteador
byte subnet[] = { 255, 255, 255, 0 };// 255, 255, 255, 0

EthernetClient client;
EthernetServer server(8095);

int ledRed = 8;//se estiver aceso esta desconectado
int ledGreen = 9;//se estiver aceso esta conectado

int exaustor = 2;
int ventilador = 3;
int lampada = 4;
int nebulizador = 5;
int aquecedor = 26; // pino 26 do Mega
int rele1 = 51;//esse rele selecionará a fonte de energia, se o valor dele for 0, libera a energia da rede elétrica, se for 1 libera a energia do backup(baterias);
int sensorCorrente = A1;

// Definindo pinos do sensor mq7
int pin_digital_mq7 = 7;
int pin_mq7 = A2;

int valor_analogico_mq7;
int valor_analogico_mq135;

// Definindo pinos do sensor mq135
int mq135 = A0;

// variaveis para sensor DHT11
float temperaturaDHT11 = 0;
float umidadeDHT11 = 0;
DHT dht(6, DHT11);//passando como parametro o pino e modelo do sensor

// variaveis para sensor LDR/luminosidade
int luminosidadeLdr = 0;

//PARA TESTAR
int controle = 0;//essa variável vai dizer se o sistema ta no modo manual ou automático
String acao; // recebe a string de comando enviada pela página controle ou altera dados
float sensibilidade = 0.100; // variavel para auxiliar no calculo da corrente


// String onde é guardada as msgs recebidas
char msg[7] = "0000L#";

// String que representa o estado dos dispositivos
char Comando[7] = "0000L#";

String readString = String(30);

unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change :
const long interval = 1000;           // interval at which to blink (milliseconds)

#define req_bufer_size 50
char HTTP_request[req_bufer_size] = {0};//variavel que armazena  o q o cliente ta pedindo de request, solicitação
char req_index = 0;// variavel que me ajuda a saber cada caracter do HTTP_request

void(* resetFunc) (void) = 0;//será usada para resetar o arduino via firmware

//variaveis que podem ter seus valores alterados via formulario php

int tempMax = 29; //essa variavel diz para o at mega a temperatura maxima tolerada, passando disso aciona exaustores
int tempMin = 27; //essa variavel diz para o at mega a temperatura minima tolerada, menor que isso aciona aquecedores
int umiMax = 75; //essa variavel diz para o at mega a umidade maxima tolerada, passando disso aciona ventiladores
int umiMin = 60; //essa variavel diz para o at mega a umidade minima tolerada, menos q isso aciona nebulizadores
int lumMin = 70; //essa variavel diz para o at mega a luminosidade minima tolerada, menos q isso aciona lampadas
int coMax = 50; //essa variavel diz para o at mega a qtd maxima em ppm de co tolerado, passando disso aciona ventiladores
int amoMax = 50; //essa variavel diz para o at mega a qtd maxima em ppm de nh3 tolerado, passando disso aciona ventiladores


void setup() {
  Ethernet.begin(mac,ipClient,gateway,subnet);
  Serial.begin(9600);
  server.begin();
  // Define os pinos de leitura do sensor como entrada
  pinMode(pin_digital_mq7, INPUT);
  pinMode(pin_mq7, INPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  digitalWrite(ledGreen, LOW);
  digitalWrite(ledRed, HIGH);
  
  pinMode(exaustor,OUTPUT);
  pinMode(ventilador,OUTPUT);
  pinMode(lampada,OUTPUT);
  pinMode(nebulizador,OUTPUT);
  pinMode(aquecedor,OUTPUT);
  pinMode(rele1,OUTPUT);
  pinMode(A4,OUTPUT);
  pinMode(A5,OUTPUT); 
  
  digitalWrite(rele1,LOW);
 
  //configurando o modulo clock
  /* rtc.halt(false);//acionando o relogio
  rtc.setDOW(FRIDAY);//definindo dia da semana
  rtc.setTime(10,39,0);//definindo o horário
  rtc.setDate(6,10,2016);//definindo dia, mes e ano
  rtc.setSQWRate(SQW_RATE_1);
  rtc.enableSQW(true);*/
}

void loop() {


  Serial.print("TempMax: ");
  Serial.print(tempMax);

  Serial.print("TempMin: ");
  Serial.print(tempMin);

  Serial.print("UmiMin: ");
  Serial.print(umiMax);

  Serial.print("UmiMin: ");
  Serial.print(umiMin);

  Serial.print("CoMax: ");
  Serial.print(coMax);

  Serial.print("AmoMax: ");
  Serial.print(amoMax);

  Serial.print("LumMin: ");
  Serial.print(lumMin);



  
  EthernetClient client = server.available();
  unsigned long currentMillis = millis();

  comando(client);


  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    if (client.connect(ipServer, 8095)) {
      digitalWrite(ledGreen, HIGH);
      digitalWrite(ledRed, LOW);  
      Serial.println("Conectado");
      //boolean currentLineIsBlank = true;//vai verificar e o cliente ja parou de enviar requisições
      // Make a HTTP request:
      enviaDadosBanco(client);
      
  } 
  else {
    // if you didn't get a connection to the server:
    digitalWrite(ledGreen, LOW);
    digitalWrite(ledRed, HIGH);  
    Serial.println("Falha na Conexão");
    //client.stop();
  }
  }
}


//FUNÇÕES LENDO OS SENSORES

void MQ7(){//essa função lê e imprime na prota serial o valor do sensor mq7/CO
     // Le os dados do pino digital D0 do sensor
    int valor_digital = digitalRead(pin_digital_mq7);
    // Le os dados do pino analogico A0 do sensor
    valor_analogico_mq7 = analogRead(pin_mq7);
    // Mostra os dados no serial monitor
    /*Serial.print("Pino D0 : ");
    Serial.println(valor_digital);
    Serial.print("Pino A0 : ");
    Serial.println(valor_analogico_mq7);  
    Serial.println("---------------------------------------"); */
    delay(10);
}

void MQ135(){//essa função lê e imprime na prota serial o valor do sensor mq7/CO
    valor_analogico_mq135 = analogRead(mq135);
    /*Serial.println("LEITURA DO AR MQ135");
    Serial.print(valor_analogico_mq135);
    Serial.println(" ppm");
    Serial.println("---------------------------------------");*/
    delay(10);
}

void sensorDHT11(){//essa função lê e imprime na prota serial o valor do sensor DHT11
    temperaturaDHT11 = dht.readTemperature();//ler a temperatura
    umidadeDHT11 = dht.readHumidity();// ler a umidade
    //Imprimindo dados na porta serial
    /*Serial.print("Temperatura: ");
    Serial.println(temperaturaDHT11);
    Serial.print("Humidade: ");
    Serial.println(umidadeDHT11);
    Serial.println("---------------------------------------");*/
    delay(10);
}

void LDR(){//essa função lê e imprime na prota serial o valor do sensor LDR
    luminosidadeLdr = analogRead(A3);//conectar meusensor de luminosidade no analogio 0;
    luminosidadeLdr = map(luminosidadeLdr,0,1023,0,100);
    /*Serial.println("luminosidade: ");
    Serial.println(luminosidadeLdr);
    Serial.println("---------------------------------------");*/
    delay(10);
}


void enviaDadosBanco(EthernetClient client ){// envia os dados lidos pelo arduino para o banco de dados atraves do servidor
    MQ7();
    MQ135();
    sensorDHT11();
    LDR();

    client.print("GET /TCCII/_php/salvaDados.php?");
    client.print("&sensorDht11Temp=");
    client.print(temperaturaDHT11);
    client.print("&sensorDht11Umidade=");
    client.print(umidadeDHT11);
    client.print("&sensorMq7=");
    client.print(valor_analogico_mq7);    
    client.print("&sensorMq135=");
    client.print(valor_analogico_mq135);
    client.print("&sensorLdr=");
    client.println(luminosidadeLdr);
    client.stop();
}

void comando(EthernetClient client){
  // SE receber um caracter...


  if(controle == 0){//variavel controle igual a 0 
     acionaAtuadores();// deixa o sistema no modo automatico
  }
  
  if(client)
  {
 
    boolean continua = true;
    String linha = "";
    
    while(client.connected())
    {          
      //recebe parametros da aplicação web que altera as variaveis de ambiente do arduino
      
      if(client.available()){
        char c = client.read();
        linha.concat(c);
  
        if(c == '\n' && continua)
        {
          client.println("HTTP/1.1 200 OK");
          // IMPORTANTE, ISSO FAZ O ARDUINO RECEBER REQUISIÇÃO AJAX DE OUTRO SERVIDOR E NÃO APENAS LOCAL.
          client.println("Content-Type: text/javascript");
          client.println("Access-Control-Allow-Origin: *");
          client.println();          

          //ALTERADO 29-01-2017
          int inicio = linha.indexOf("=") + 1;//ate o 60, pegando temperatura max
          int fim = inicio + 1;
          String v = linha.substring(inicio, fim);

          if(v == "a"){//se o primeiro caracter depois do "=" for igual a "a" he porque o comando esta vindo da pagina "altera parametros do sistema" se for diferente ta vindo  
              alteraParametros(linha); 
          }else{
            
            int iniciofrente = linha.indexOf("?");
                
            if(iniciofrente>-1){     //verifica se o comando veio
              iniciofrente     = iniciofrente+6; //pega o caractere seguinte
              int fimfrente    = iniciofrente+3; //esse comando espero 3 caracteres
              acao    = linha.substring(iniciofrente,fimfrente);//recupero o valor do comando  //acao agora é variavel publica
              //parametros[i] = acao;//alterado por mim 12-01-2017 se der errado remover
             // Serial.print("LINHA: ");
             // Serial.println(linha);
              
         
              if(acao == "000"){//altera o estado do sistema para manual quando a variavel controle for igual a 1 e automatico quando for igual a 0
                if(controle == 0){//
                  controle = 1;
                }else{
                  controle = 0;
                }
              }
              if(controle == 1){//deixa o sistema no modo manual, controlado pela pagina controle
                  if( acao == "001"){//controle manual
                    int res = digitalRead(ventilador);
                    if(res == HIGH){
                      digitalWrite(ventilador, LOW); 
                   }else{
                      digitalWrite(ventilador, HIGH); 
                    }            
                  }else if( acao == "002"){
                    int res1 = digitalRead(nebulizador);
                     if(res1 == HIGH){
                       digitalWrite(nebulizador, LOW);
                     }else{
                       digitalWrite(nebulizador, HIGH);
                     }
                  }else if( acao == "003"){
                    int res2 = digitalRead(lampada);
                     if(res2 == HIGH){
                       digitalWrite(lampada, LOW);
                     }else{
                       digitalWrite(lampada, HIGH);
                     }
                  }else if( acao == "004"){
                    int res3 = digitalRead(aquecedor);
                     if(res3 == HIGH){
                       digitalWrite(aquecedor, LOW);
                     }else{
                       digitalWrite(aquecedor, HIGH);
                     }
                  }
                  else if( acao == "005"){
                    int res4 = digitalRead(exaustor);
                     if(res4 == HIGH){
                       digitalWrite(exaustor, LOW);
                     }else{
                       digitalWrite(exaustor, HIGH);
                     }
                  }
                }
            }
              client.print("dados({ ventilador : ");
              client.print(digitalRead(ventilador));
              client.print(", nebulizador :  ");
              client.print(digitalRead(nebulizador));
              client.print(",");
              client.print(" lampada : ");
              client.print(digitalRead(lampada));
              client.print(",");
              client.print(" aquecedor : ");
              client.print(digitalRead(aquecedor));
              client.print(",");
              client.print(" exaustor : ");
              client.print(digitalRead(exaustor));
              client.print(",");
              client.print(" controle : ");
              client.print(controle);
              client.print(",");
              client.print(" seletEnergia : ");           
              client.print(digitalRead(rele1));//vai dizer qual a fonde de energia no momento
              client.print("})");
          

          
          

          //FIM ALTERACAO 29-01-2017

         
         /* int iniciofrente = linha.indexOf("?");
                
          if(iniciofrente>-1){     //verifica se o comando veio
            iniciofrente     = iniciofrente+6; //pega o caractere seguinte
            int fimfrente    = iniciofrente+3; //esse comando espero 3 caracteres
            String acao    = linha.substring(iniciofrente,fimfrente);//recupero o valor do comando
            //parametros[i] = acao;//alterado por mim 12-01-2017 se der errado remover
            Serial.print("LINHA: ");
            Serial.println(linha);
            
       
            if(acao == "000"){//altera o estado do sistema para manual quando a variavel controle for igual a 1 e automatico quando for igual a 0
              if(controle == 0){//
                controle = 1;
              }else{
                controle = 0;
              }
            }
            if(controle == 1){//deixa o sistema no modo manual, controlado pela pagina controle
                if( acao == "001"){//controle manual
                  int res = digitalRead(ventilador);
                  if(res == HIGH){
                    digitalWrite(ventilador, LOW); 
                 }else{
                    digitalWrite(ventilador, HIGH); 
                  }            
                }else if( acao == "002"){
                  int res1 = digitalRead(nebulizador);
                   if(res1 == HIGH){
                     digitalWrite(nebulizador, LOW);
                   }else{
                     digitalWrite(nebulizador, HIGH);
                   }
                }else if( acao == "003"){
                  int res2 = digitalRead(lampada);
                   if(res2 == HIGH){
                     digitalWrite(lampada, LOW);
                   }else{
                     digitalWrite(lampada, HIGH);
                   }
                }else if( acao == "004"){
                  int res3 = digitalRead(aquecedor);
                   if(res3 == HIGH){
                     digitalWrite(aquecedor, LOW);
                   }else{
                     digitalWrite(aquecedor, HIGH);
                   }
                }
                else if( acao == "005"){
                  int res4 = digitalRead(exaustor);
                   if(res4 == HIGH){
                     digitalWrite(exaustor, LOW);
                   }else{
                     digitalWrite(exaustor, HIGH);
                   }
                }
            }else{
                 acionaAtuadores();// deixa o sistema no modo automatico
            }
            
            client.print("dados({ ventilador : ");
            client.print(digitalRead(ventilador));
            client.print(", nebulizador :  ");
            client.print(digitalRead(nebulizador));
            client.print(",");
            client.print(" lampada : ");
            client.print(digitalRead(lampada));
            client.print(",");
            client.print(" aquecedor : ");
            client.print(digitalRead(aquecedor));
            client.print(",");
            client.print(" exaustor : ");
            client.print(digitalRead(exaustor));
            client.print(",");
            client.print(" controle : ");
            client.print(controle);
            client.print(",");
            client.print(" seletEnergia : ");           
            client.print(digitalRead(rele1));//vai dizer qual a fonde de energia no momento
            client.print("})");
            */
         }
               
                   
          break;
        }
        if(c == '\n') { continua = true; }
        else if (c != '\r') { continua = false; }
      }
    }
     delay(1);
     client.stop();
}
}

void acionaUmidade(){//acionara os reles dos atuadores que controlam a umidade
  if((umidadeDHT11 < umiMin) && (temperaturaDHT11 < tempMin) && (pin_mq7 < coMax) && (mq135 < amoMax)){
    digitalWrite(nebulizador, HIGH);
    digitalWrite(ventilador, LOW);
  }
  if(umidadeDHT11 > umiMax){
    digitalWrite(ventilador, HIGH);
    digitalWrite(nebulizador, LOW);
  }if(((umidadeDHT11 > umiMin) && (umidadeDHT11 < umiMax) && (temperaturaDHT11 > tempMin) && (temperaturaDHT11 < tempMax)) &&((umidadeDHT11 < umiMin) && (temperaturaDHT11 < tempMin) && (pin_mq7 < coMax) && (mq135 < amoMax))){
    digitalWrite(nebulizador, LOW);
    digitalWrite(ventilador, LOW);
  }
}

void acionaTemp(){//acionara os reles dos atuadores que controlam os parametros temperatura
  if((temperaturaDHT11 < tempMin)){
    digitalWrite(aquecedor, HIGH);
    digitalWrite(exaustor, LOW);
  }if(temperaturaDHT11 > tempMax){
    digitalWrite(ventilador, HIGH);
    digitalWrite(exaustor, HIGH);
    digitalWrite(aquecedor, LOW);
  }if((temperaturaDHT11 > tempMin) && (temperaturaDHT11 < tempMax)){
    digitalWrite(aquecedor, LOW);
    digitalWrite(exaustor, LOW);
  }
  if(((temperaturaDHT11 > tempMin) && (temperaturaDHT11 < tempMax) && (umidadeDHT11 > umiMin) && (umidadeDHT11 < umiMax)) &&((umidadeDHT11 < umiMin) && (temperaturaDHT11 < tempMin) && (pin_mq7 < coMax) && (mq135 < amoMax))){
    digitalWrite(ventilador, LOW);   
  }
}

//FUNCÕES PARA ACIONA ATUADORES

void acionaCo(){//aciona os reles dos atuadores que controlam os parametros monoxido de carbono
  if(pin_mq7 > coMax){
    digitalWrite(ventilador, HIGH);
  }if((pin_mq7 < coMax) && (mq135 < amoMax) && (temperaturaDHT11 < tempMin) && (umidadeDHT11 < umiMin)){
    digitalWrite(ventilador, LOW);
  }
}

void acionaNh3(){//aciona os reles dos atuadores que controlam os parametros amonia
  if(mq135 > amoMax){
    digitalWrite(ventilador, HIGH);
  }if((mq135 < amoMax) && (pin_mq7 < coMax) && (temperaturaDHT11 < tempMin) && (umidadeDHT11 < umiMin)){
    digitalWrite(ventilador, LOW);
  }
}

void acionaLuz(){//aciona os reles dos atuadores que controlam os parametros amonia
  if(luminosidadeLdr < lumMin){
    digitalWrite(lampada, HIGH);
  }else{
      digitalWrite(lampada, LOW);
  }
}

void acionaAtuadores(){//executa as funcões q acionam os atuadores
  acionaLuz();
  acionaNh3();
  acionaCo();
  acionaTemp();
  acionaUmidade();
}

void selectFontPower(){ //faltachamar essa função na loop
  if(calculaCorrente() < 0.05){
    digitalWrite(rele1, HIGH);
  }
  else{
    digitalWrite(rele1, LOW);
  }
}

float calculaCorrente(){
  float lerACS712 = 0;
  float intensidade = 0;
  for(int i=0; i < 500; i++){
    lerACS712 = analogRead(sensorCorrente) + (5.02/1023.0);
    intensidade = intensidade + (lerACS712-2.5)/sensibilidade;
  }
  intensidade = intensidade/500;
  return(intensidade);
}


void alteraParametros(String linha){
    int iniciofrente2 = linha.indexOf("=") + 2;//ate o 60, pegando temperatura max
          int fimfrente2 = iniciofrente2 + 2;
          String valor1 = linha.substring(iniciofrente2, fimfrente2);
          delay(5);
          int iniciofrente3 = linha.indexOf("=") + 17; //ate o 28, pegando temperatura min
          int fimfrente3 = iniciofrente3 + 2;
          String valor2 = linha.substring(iniciofrente3, fimfrente3);
          delay(5);  
          int iniciofrente4 = linha.indexOf("=") + 32; //ate o 75, pegando umidade max
          int fimfrente4 = iniciofrente4 + 2;
          String valor3 = linha.substring(iniciofrente4, fimfrente4);
          delay(5);
          int iniciofrente5 = linha.indexOf("=") + 47;//ate o 60, pegando umidade min
          int fimfrente5 = iniciofrente5 + 2;
          String valor4 = linha.substring(iniciofrente5, fimfrente5);
          delay(5);
          int iniciofrente6 = linha.indexOf("=") + 62;//ate o 90, pegando co
          int fimfrente6 = iniciofrente6 + 2;
          String valor5 = linha.substring(iniciofrente6, fimfrente6);
          delay(5);
          int iniciofrente7 = linha.indexOf("=") + 77;//ate o 60, pegando nh3
          int fimfrente7 = iniciofrente7 + 2;
          String valor6 = linha.substring(iniciofrente7, fimfrente7);
          delay(5);
          int iniciofrente8 = linha.indexOf("=") + 92;//ate o 40, pegando luminosidade
          int fimfrente8 = iniciofrente8 + 2;
          String valor7 = linha.substring(iniciofrente8, fimfrente8);

          tempMax = valor1.toInt(); 
          tempMin = valor2.toInt(); 
          umiMax = valor3.toInt(); 
          umiMin = valor4.toInt(); 
          coMax = valor5.toInt(); 
          amoMax = valor6.toInt(); 
          lumMin = valor7.toInt(); 
          
          Serial.print("TempMax: ");
          Serial.print(tempMax);

          Serial.print("TempMin: ");
          Serial.print(tempMin);

          Serial.print("UmiMax: ");
          Serial.print(umiMax);

          Serial.print("UmiMin: ");
          Serial.print(umiMin);

          Serial.print("CoMax: ");
          Serial.print(coMax);

          Serial.print("AmoMax: ");
          Serial.print(amoMax);

          Serial.print("LumMin: ");
          Serial.print(lumMin);

          
          /*Serial.print("VALOR1: ");
          Serial.println(valor1);
          Serial.print("VALOR2: ");
          Serial.println(valor2);
          Serial.print("VALOR3: ");
          Serial.println(valor3);
          Serial.print("VALOR4: ");
          Serial.println(valor4);
          Serial.print("VALOR5: ");
          Serial.println(valor5);
          Serial.print("VALOR6: ");
          Serial.println(valor6);
          Serial.print("VALOR7: ");
          Serial.println(valor7);
          Serial.print("INICIOFRENTE2: ");
          Serial.println(linha);*/
}

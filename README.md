# Proyecto DACTEA
![Logo](Fotos/logo)

Consta de la creación de un equipo médico el cual manda señales biomédicas a Arduino. Estas señales se registran y según condiciones umbral, se dispara una respuesta. La activación del equipo queda registrado en un website realizado con los lenguajes html/css/js/python. 

1. En Visual Studio Code bajar la extensión Live Server.
2. Ir a _File_ y seleccionar la opción _Open Folder_, abrir la carpeta DACTEA-website. Dentro de la carpeta se encuentran:
  - index.html
      - Sitio web.
  - carpeta assets
      - css - Contiene archivos correspondientes a la estética visual del sitio. 
      - images - Imágenes accesorias utilizadas en el sitio.
      - js
          - Scrip de funcionalidad.
          - Contiene las funciones de procesamiento de bpm y rpm, la conexión a Arduino, y la creación del .csv.
  - guardarCSV.py - Código encargado de la creación del archivo EjemploRegistro.csv y la carga de datos según Arduino lo indique. 
3. A la izquierda de la barra inferior, seleccionar _Go Live_. Esto ejecutará la página web.
4. Correr el archivo DACTEA-arduino.ino en Arduino IDE.
5. Correr el archivo guardarCSV.py en Visual Studio Code (con control+C genera una interrupción a partir de la cual se actualiza el archivo Registro.csv y se visualizan los datos en la tabla del sitio web).

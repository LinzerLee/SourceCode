javac -d class com/fourinone/*.java
copy com\fourinone\config_en_US.properties class\META-INF
copy com\fourinone\fttp.jsp class\WEB-INFO
copy com\fourinone\err401.jsp class\WEB-INFO
copy com\fourinone\err404.jsp class\WEB-INFO
copy com\fourinone\fttp.js class\WEB-INFO
rem copy com\fourinone\*.java class\com\fourinone\
cd class
jar -cf fourinone.jar META-INF WEB-INFO com/fourinone/*.*
cd ..
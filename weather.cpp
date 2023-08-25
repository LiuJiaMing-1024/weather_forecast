﻿#include "weather.h"
#include "ui_weather.h"
#include <string>
#include <QTimer>
using std::string;
using namespace QtCharts;
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{


    ui->setupUi(this);
    //设置无边框
    setWindowFlag(Qt::FramelessWindowHint);
    setFixedSize(width(),height());

    //添加退出功能
    menu = new QMenu(ui->widget);
    exit = new QAction(QIcon(":/weatherIco/close.ico"),QString("exit"),menu);
    menu->addAction(exit);
    connect(exit,SIGNAL(triggered(bool)),this,SLOT(exit_fun()));

    ui->city_search->setStyleSheet("QLineEdit{border: 1px solid gray; border-radius: 4px; background:argb(47, 47, 47, 130); color:rgb(0, 0, 0);} QLineEdit:hover{border-color:rgb(101, 255, 106); }");

    //添加ui控件
    forecast_week_list << ui->datecn1 << ui->datecn2 << ui->datecn3 << ui->datecn4 << ui->datecn5 << ui->datecn6;
    forecast_date_list << ui->datenum1 << ui->datenum2 << ui->datenum3 << ui->datenum4 << ui->datenum5 << ui->datenum6;
    forecast_type_list << ui->type1 << ui->type2 << ui->type3 << ui->type4 << ui->type5 << ui->type6;
    forecast_typeIco_list << ui->typeicon1 << ui->typeicon2 << ui->typeicon3 << ui->typeicon4 << ui->typeicon5 << ui->typeicon6;
    forecast_high_list << ui->temperature_max1 << ui->temperature_max2 << ui->temperature_max3 << ui->temperature_max4 << ui->temperature_max5 << ui->temperature_max6;
    forecast_low_list << ui->temperature_min1 << ui->temperature_min2 << ui->temperature_min3 << ui->temperature_min4 << ui->temperature_min5 << ui->temperature_min6;

    // dateLb和WeekLb样式表设置
    for (int i = 0; i < 6; i++)
    {
        forecast_date_list[i]->setStyleSheet("background-color: rgba(0, 191, 255, 100);");
        forecast_week_list[i]->setStyleSheet("background-color: rgba(0, 191, 255, 100);");
        forecast_type_list[i]->setStyleSheet("background-color: rgba(0, 191, 255, 100);");
        forecast_high_list[i]->setStyleSheet("background-color: rgba(0, 191, 255, 100);");
        forecast_low_list[i]->setStyleSheet ("background-color: rgba(0, 191, 255, 100);");
        forecast_typeIco_list[i]->setStyleSheet("background-color: rgba(255, 255, 255, 90);");
        forecast_typeIco_list[i]->setMinimumSize(100,100);
    }
    ui->line_chart->setStyleSheet("background-color: rgba(192, 192, 192, 100);");
    WeatherTool tool;

    manager = new QNetworkAccessManager(this);
    for(int i=0;i<6;i++){
        image_manager[i] =  new QNetworkAccessManager(this);
        // 连接信号与槽，处理请求完成后的响应
        connect(image_manager[i],&QNetworkAccessManager::finished,
                this,
                bind(&Widget::setImage,this,std::placeholders::_1,i));
    }
    connect(manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(parseRequest(QNetworkReply*)));
    today_url = u8"https://jisutqybmf.market.alicloudapi.com/weather/query";
    forecast_url = u8"https://jmweather.market.alicloudapi.com/weather/query/15d";
    city=u8"广州";
    getWeatherInfo();

    //安装事件过滤器
    ui->sunarc->installEventFilter(this);
    ui->line_chart->installEventFilter(this);
    timer=new QTimer(ui->sunarc);
    connect(timer,SIGNAL(timeout()),ui->sunarc,SLOT(update()));
    timer->start(1000);



}

//解析天气预报数据(json)，初始化today以及forecast成员
void Widget::parseJson(const QJsonObject& jsonObj){

    if(jsonObj.contains("result"))
        today = jsonObj["result"].toObject();
    else if(jsonObj.contains("data")){
        for(int i=0;i<6;i++){
            forecast[i] = jsonObj["data"].toObject()["dayWeathers"].toArray().at(i).toObject();
        }
    }
    setUI();
}


//解析http请求，获取Json数据
void Widget::parseRequest(QNetworkReply * reply){
    qDebug()<<u8"parseRequest";
    //获取响应状态码
    QVariant status_code =reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    QByteArray data = reply->readAll();
    QJsonParseError err;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data,&err);
    QJsonValue msg;
    //状态码不正确
    if(reply->error()!=QNetworkReply::NoError||status_code!=200){
        msg = jsonDoc[u8"msg"];
        qDebug()<<msg.toString();
        QMessageBox::warning(this,u8"警告",u8"获取天气数据失败！",QMessageBox::Ok,QMessageBox::No);
        return;
    }
    msg = jsonDoc[u8"msg"];
    qDebug()<<msg.toString();
    parseJson(jsonDoc.object());

}


void Widget::setImage(QNetworkReply *reply,int index){
    if (reply->error() == QNetworkReply::NoError) {
        QImage image;
        // 获取响应数据
        QByteArray imageData = reply->readAll();
        // 将数据转换为QImage
        image.loadFromData(imageData);

        forecast_typeIco_list[index]->setPixmap(QPixmap::fromImage(image));
        if(0==index)
            ui->type_icon->setPixmap(QPixmap::fromImage(image));
    }
    reply->deleteLater();
}


void Widget::paintSunSetUI(){
    //绘制底部直线以及圆弧
    QPainter painter;
    painter.setPen(Qt::yellow);
    //设置绘画的起点为ui->sunarc控件
    painter.begin(ui->sunarc);

    painter.drawLine(ui->sunarc->width()*0.1,
                     ui->sunarc->height()*0.7,
                     ui->sunarc->width()*0.9,
                     ui->sunarc->height()*0.7
                     );

    QRect rect(ui->sunarc->width()*0.1,ui->sunarc->height()*0.72,ui->sunarc->width()*0.8,ui->sunarc->height()*0.28);

    QTime cur_time = QDateTime::currentDateTime().time();
    QTime sunrise = QTime::fromString(today.sunrise,"HH:mm");
    QTime sunset = QTime::fromString(today.sunset,"HH:mm");

    painter.drawText(rect,Qt::AlignCenter|Qt::AlignVCenter,u8"日出日落");
    painter.drawText(rect,Qt::AlignLeft|Qt::AlignVCenter,sunrise.toString("HH:mm"));
    painter.drawText(rect,Qt::AlignRight|Qt::AlignVCenter,sunset.toString("HH:mm"));

    rect = QRect(ui->sunarc->width()*0.1, 0 ,ui->sunarc->width()*0.8,ui->sunarc->height()*1.4);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawArc(rect,0*16,180*16);

    int sunset_total_minutes = sunset.hour()*60+sunset.minute();
    int sunrise_total_minutes = sunrise.hour()*60+sunrise.minute();
    int cur_minutes = cur_time.hour()*60+cur_time.minute();

    double start_angle;
    double span_angle;
    if(sunset_total_minutes<cur_minutes){
        start_angle=0;
        span_angle = 180*16;
    }else{
        start_angle = (sunset_total_minutes-cur_minutes)*1.0/(sunset_total_minutes-sunrise_total_minutes)*180 *16;
        span_angle = (cur_minutes-sunrise_total_minutes)*1.0/(sunset_total_minutes-sunrise_total_minutes)*180*16;
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 85, 0, 100));
    painter.drawPie(rect,start_angle,span_angle);
    painter.end();
}

//设置UI界面信息
void Widget::setUI(){
    //设置today的ui界面
    ui->datelb->setText(today.date);
    ui->temperature->setText(today.wendu);
    ui->quality_lb->setText(today.quality);
    ui->notice_lb->setText(today.notice);
    ui->location_text->setText(today.city);
    ui->wendu->setText(today.wendu);
    ui->shidu->setText(today.shidu);
    ui->pm2_5->setText(today.pm25);
    ui->fengli->setText(today.fl);
    ui->ganmao->setText(today.ganmao);

    //设置未来6天的ui界面
    for(int i=0;i<6;i++){
        QDate cur_date = QDate::fromString(forecast[i].date,"yyyyMMdd");
        forecast_date_list[i]->setText(cur_date.toString("MM/dd"));
        forecast_week_list[i]->setText(date2weekday(cur_date.dayOfWeek()));
        forecast_low_list[i]->setText(forecast[i].low);
        forecast_high_list[i]->setText(forecast[i].high);
        forecast_type_list[i]->setText(forecast[i].type);
        forecast_typeIco_list[i]->setScaledContents(true);
        //设置天气图标ICON
        image_manager[i]->get(QNetworkRequest(QUrl(forecast[i].weather_url)));
    }
    ui->line_chart->update();

}

void Widget::getWeatherInfo(){
    QString city_code = tool[city];


    if(city_code==u8"00000000"){
        QMessageBox::warning(this,u8"警告",u8"不存在该城市！",QMessageBox::Ok,QMessageBox::No);
        return;
    }
//    string encode;
//    UrlEncode(city.toStdString(),encode);
//    QString area = QString(encode.c_str());
    //正式开始http请求

    //获取今日天气信息
    QNetworkRequest request(today_url);
    request.setRawHeader(u8"Authorization",u8"APPCODE 6ba0931b2d604deba881134ce9114fc5");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=utf-8");
    QByteArray requestBody = QString(u8"city="+city).toUtf8();
    manager->post(request,requestBody);

    //获取历史7天天气信息
    request.setUrl(forecast_url);
    requestBody = QString(u8"area="+city).toUtf8();
    manager->post(request,requestBody);


}

void Widget::paintLineChart(){
    QList<double> low_list;
    QList<double> high_list;
    for(int i=0;i<6;i++){
        low_list<<forecast[i].low.toDouble();
        high_list<<forecast[i].high.toDouble();
    }
    auto it = std::min_element(low_list.begin(),low_list.end());
    double min = *it;
    it = std::max_element(high_list.begin(),high_list.end());
    double max = *it;
    double block_width = ui->line_chart->width()/6;

    if(int(min)){
        qDebug()<<min<<max;
        qDebug()<<u8"开始画直线图";
        QPainter painter;
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::yellow);
        painter.begin(ui->line_chart);
        painter.translate(block_width/2,ui->line_chart->height()*0.1);

        //开始画图
        QPoint prev_low(0 , (1.0-((low_list[0]-min)/(max-min))) * ui->line_chart->height()*0.8);
        QPoint prev_high(0 , (1.0-((high_list[0]-min)/(max-min))) * ui->line_chart->height()*0.8);
        painter.setBrush(Qt::red);


        QPen pen;
        pen.setCapStyle(Qt::FlatCap);
        painter.setPen(Qt::DashDotDotLine);
        painter.drawEllipse(prev_low, 2, 2);
        painter.drawEllipse(prev_high, 2, 2);

        for(int i=1;i<low_list.size();i++){

            QPoint cur_low = QPoint(prev_low.x()+block_width,(1.0-((low_list[i]-min)/(max-min))) * ui->line_chart->height()*0.8);

            painter.drawLine(prev_low,cur_low);

            prev_low=cur_low;


            QPoint cur_high = QPoint(prev_high.x()+block_width,(1.0-((high_list[i]-min)/(max-min))) * ui->line_chart->height()*0.8);

            painter.drawLine(prev_high,cur_high);
            prev_high=cur_high;

            painter.drawEllipse(cur_low, 2, 2);
            painter.drawEllipse(cur_high, 2, 2);

        }
        painter.end();
    }



}



//右键弹出菜单
void Widget::contextMenuEvent(QContextMenuEvent *event){
    menu->exec(QCursor::pos());
    event->accept();
}

//鼠标左击更新坐标值
void Widget::mousePressEvent(QMouseEvent *event){
    cur_pos = event->globalPos()-pos();
}

//鼠标左击并移动更新窗体位置
void Widget::mouseMoveEvent(QMouseEvent *event){
    move(event->globalPos()-cur_pos);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::exit_fun(){

    qApp->exit(0);
}



bool Widget::eventFilter(QObject *watched, QEvent *event){
    if(watched==ui->sunarc&&event->type()==QEvent::Paint){
        paintSunSetUI();
    }
    if(watched==ui->line_chart&&event->type()==QEvent::Paint){
        paintLineChart();
    }
    return QObject::eventFilter(watched,event);
}

void Widget::on_search_btn_clicked(bool checked)
{
    QString temp_city = ui->city_search->text();
    if(tool[temp_city]!="000000000")
        city = temp_city;
        getWeatherInfo();


}


void Widget::on_refresh_btn_clicked(bool checked)
{
    getWeatherInfo();
}


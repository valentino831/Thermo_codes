close all
clear all
clc

v = FlirMovieReader('120000.ats');
v.unit='temperatureFactory';
[frame, metadata] = step(v);
% In metadata hai i parametri della termocamera (frequenza e cose del
% genere)
% Traccio il grafico dell'andamento della temperatura T(t), dove T è
% calcolato come la media delle temperature nella regione (50,43)-(60,58)

frame = wiener2(double(frame), [5 5]);
T = mean(frame(50:60,43:58),'all');
while ~isDone(v)
    frame = step(v);
    frame = wiener2(double(frame), [5 5]);
    T = [T;mean(frame(50:60,43:58),'all')];
end

fc = 20;
fs = metadata.FrameRate;

[b,a] = butter(6,fc/(fs/2));

T1=filtfilt(b,a,T);


t = (0:length(T)-1)/metadata.FrameRate;
figure, plot(t, T)
hold on 
plot(t,T1)
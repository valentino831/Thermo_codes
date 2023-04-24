close all
clear all
clc

%!!!!aggiungere al path la cartella script aggiornata!!!!!! 

s       = pathsep;
pathStr = [s, path, s];
onPath  = contains(pathStr, [s, pwd '\script', s], 'IgnoreCase', ispc);
if ~onPath
    addpath('script');
end


% folder in cui ci sono i file ats (cambiala per il tuo caso) 
atsDir = 'C:\Users\d016781\Dropbox (Politecnico Di Torino Studenti)\Termografia_Santoro\Razza_0210';

% folder in cui salvare tutto (cambiala per il tuo caso)
saveDir = 'C:\Users\d016781\Dropbox (Politecnico Di Torino Studenti)\Termografia_Santoro\Razza_0210\saves';
xlsFile = 'nomeFile.xls';

% salvo il file excel nella cartella dei salvataggi (saveDir)
xlsFile = fullfile(saveDir, xlsFile);

searchFile = fullfile(atsDir,'*.ats');
lista = dir(searchFile);

for fileIdx = 1:length(lista)
    fileAts = fullfile(lista(fileIdx).folder, lista(fileIdx).name);
    baseName = lista(fileIdx).name(1:find(lista(fileIdx).name == '.',1)-1);
    ta = TermoAnalizer(fileAts, fullfile(saveDir,baseName));

    [frame_start,frame_end]=ta.cercaPeriodo(100); 

    freq=2;
    ta.LockInAmplifier(freq,frame_start,frame_end)

    mmpxratio=30/240;%% 
    tol=0.1;
    % tol2=0.05, 
    
    % [Cut_x,yp,Cut_y,xp] =  ta.LockinResults(freq,xc,yc,mmpxratio,tol)
    [Cut_x,yp,Cut_y,xp]=ta.LockinAmplifierResults(freq,mmpxratio,tol,1);

    data={lista(fileIdx).name(1:find(lista(fileIdx).name == '.',1)-1), ...
        55, 10};

    xlswrite(xlsFile, data, 'Foglio 1', ['A' num2str(fileIdx)]);

end

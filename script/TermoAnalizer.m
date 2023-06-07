classdef TermoAnalizer < handle
    %TermoAnalizer Classe per analizzare i dati da FLIR
    %   Carica e analizza i dati da FLIR

    properties(Access = public)
        temp
        radiance
        metadata
        time
        framerate
        A
        P
        f_c2
        saveDir
    end

    methods
        function obj = TermoAnalizer(fileName,saveDir)
            %TermoAnalizer Construct an instance of this class
            %   Vuole il nome del file ATS da leggere
            %   saveDir se specificato, indica il nome della cartella in
            %   cui salvare le figure

            if ~exist("saveDir", "var")
                saveDir = '.';
            end
            if ~exist(saveDir, "dir")
                mkdir(saveDir);
            end
            obj.saveDir = saveDir;

            v = FlirMovieReader(fileName);
            v.unit = 'radianceFactory';
            [frame, metadata] = step(v);
            obj.metadata = metadata;

            obj.radiance = double(frame);
            tempo = str2double(metadata.Time(5:6))*60*60+str2double(metadata.Time(8:9))*60+str2double(metadata.Time(11:end));

            while ~isDone(v)
                [frame, metadata] = step(v);
                tempo = [tempo, ...
                    str2double(metadata.Time(5:6))*60*60+str2double(metadata.Time(8:9))*60+str2double(metadata.Time(11:end))];
                obj.radiance(:,:,end+1) = double(frame);
            end
            v.reset();
            obj.time = tempo-tempo(1);
            %figure, plot(tempo(2:end)-tempo(1:end-1));
            v.unit = 'temperatureFactory';
            [frame, metadata] = step(v);
            obj.temp = double(frame);

            while ~isDone(v)
                frame = step(v);
                obj.temp(:,:,end+1) = double(frame);
            end
        end

        function [tIni, tEnd] = cercaPeriodo(obj, nframe)
            %cercaPeriodo cerca l'inizio e la fine (in frame)
            %dell'eccitazione del laser
            %  nframe è il numero di frame della finestra usata nella
            %  ricerca.
            %  tIni è il primo frame in cui la temperatura inizia ad
            %  aumentare.
            %  tEnd è il frame dell'ultimo picco.

            M = max(obj.temp,[],3);
            [r,c] = find(M == max(M,[],"all"), 1);
            t = squeeze(obj.temp(r,c,:));
            
            tIni = find(t>3*sqrt(var(t(1:nframe)))+mean(t(1:nframe)), 1 );

            ii = tIni;
            jj = ii;
            fine = 0;
            while fine == 0 
                zz = ii+nframe;
                if zz > length(t)
                    zz = length(t);
                    fine = 1;
                end

                idx = find(t(ii:zz)==max(t(ii:zz)), 1, 'last' )+ii-1;
                if t(idx) >= t(jj)
                    jj = idx;
                else
                    fine = 1;
                end

                ii = zz;
            end
            tEnd = jj;
        end


        function correctfs(obj)
            %questa funzione interpola i dati acquisiti e da in output un
            %campionamento a passo costante del segnale in 1.5 volte i
            %punti del segnale in ingresso
            n = length(obj.time);
            time_equal=linspace(0,obj.time(end),1.5*n);
            obj.temp=permute(interp1(obj.time,permute(obj.temp,[3 2 1]),time_equal),[3 2 1]);
            obj.radiance=permute(interp1(obj.time,permute(obj.radiance,[3 2 1]),time_equal),[3 2 1]);
            obj.framerate=1.5*n/obj.time(end);
            obj.time=time_equal;

        end




        function temp = getTemp(obj, tIni, tEnd)
            %getTemp Restituisce la matrice delle temperature
            %  tIni è il primo frame (in numero di frame - default 1)
            %  tEnd è l'ultimo frame (in numero di frame - default l'ultimo)
            if ~exist("tEnd", "var")
                tEnd = length(obj.temp);
            end
            if tEnd > length(obj.temp)
                tEnd = length(obj.temp);
            end

            if ~exist("tIni", "var")
                tIni = 1;
            end
            temp = obj.temp(:,:,tIni:tEnd);
        end

        function cancellaFrame(obj, frame)
            %cancellaFrame rimuove un frame dalla sequenza video
            %    Nota: il frame viene tolto sia della radiance che dalla
            %    temperatura. Se cancelli un frame a metà, pensa bene a ciò
            %    che fai

            obj.radiance = obj.radiance(:,:,[1:frame-1 frame+1:end]);
            obj.temp = obj.temp(:,:,[1:frame-1 frame+1:end]);
        end



        function [maxTemp, maxTime] = getMaxTemp(obj)
            %getMaxTemp Restituisce la mappa con la temperatura massima
            %    Se non assegni il risultato a nessuna variabile, fa il
            %    plot (non apre una nuova figure)
            %    Il tempo è restituito in frame

            [temp, time] = max(obj.temp, [], 3);
            if nargout == 0
                figure
                surface(temp,'EdgeColor','none');
                view([0,-90])
            end
            if nargout > 0
                maxTemp = temp;
            end
            if nargout > 1
                maxTime = time;
            end
        end

        function normalizzaTemp(obj, temp, frames)
            %normalizzaTemp ricalcola la matrice delle temperature per
            %uniformare quella iniziale su tutti i pixel (ricalcola
            %l'emissività)
            %   temp è la temperatura iniziale che si desidera (celsius)
            %   frames è il numero di frame iniziali usati per la calcolare
            %   la normalizzazione (default 1)

            if ~exist("frames", "var")
                frames = 1;
            end

            EpsSig = mean(obj.radiance(:,:,1:frames), 3)/(temp+273.16)^4;
            obj.temp = (obj.radiance./EpsSig).^(0.25)-273.16;
        end

        function mask = tagliaMappa(obj, mask)
            %tagliaMappa toglie dalla mappa delle temperature tutti i punti
            %che non si sono scaldati
            %   mask - se è una matrice, deve avere dimensione pari al
            %   frame. Può assumere solo i valori 1 e NaN (tutti i punti
            %   diversi da NaN sono forzati ad 1). La mappa delle
            %   temperature viene settata a NaN per tutti i pixel NaN
            %   di mask. Se mask è uno scalare, viene calcolata la maschera
            %   in modo da tenere tutti i punti che hanno raggiunto una
            %   temperatura massima pari ad almeno mask volte la
            %   temperatura iniziale

            if isscalar(mask)
                th = mask;
                max = obj.getMaxTemp();
                mask = NaN(size(max));
                mask(max./obj.temp(:,:,1) >= th) = 1;
            end

            mask(~isnan(mask)) = 1;
            obj.temp = obj.temp.*mask;
        end

        function mappa = evalCooling(obj)
            %evalCooling restituisce la mappa delle costanti di tempo delle
            %curve di raffreddamento
            %   La funzione lavora sulla matrice delle temperature corrente
            %   (al netto di filtri e cose varie)

            [maxT, time] = obj.getMaxTemp(); % prendo l'istante di tempo in cui la temperatura raggiunge il massimo (pixel per pixel)

            [r,c] = size(time);

            mappa = NaN(size(time));

            for ii = 1:r
                for jj = 1:c
                    if ~isnan(maxT(ii,jj))
                        y = iddata(squeeze(obj.temp(ii,jj,time(ii,jj)+1:end)),[], 1/obj.metadata.FrameRate);
                        model = arx(y,1);
                        r = roots(model.A);
                        mappa(ii,jj) = max(abs(log(abs(r))*obj.metadata.FrameRate));
                    end
                end
            end
        end


        function mappa = evalHeating2(obj,t1,t2)
            [maxT, time] = obj.getMaxTemp(); % prendo l'istante di tempo in cui la temperatura raggiunge il massimo (pixel per pixel)

            [r,c] = size(time);

            mappa = NaN(size(time));

            Tmin=mean2(obj.temp(:,:,1));

            for ii = 1:r
                for jj = 1:c

                    T_cut=obj.temp(ii,jj,t1:t2)-Tmin;
                    %Normalizza pixel per pixel
                    T_norm=squeeze(T_cut./max(T_cut,[],3));

                    %trova i punti di meta riscaldamento
                    tMezzi = find(T_norm>=0.5,1) ;

                    mappa(ii,jj)= tMezzi-t1/obj.metadata.FrameRate;
                end
            end
            surf(mappa,EdgeColor='none')
            colorbar
            colormap("hot")
            view (2)
        end

        function mappa = evalHeatingArea(obj,t1,t2)
            [maxT, time] = obj.getMaxTemp(); % prendo l'istante di tempo in cui la temperatura raggiunge il massimo (pixel per pixel)

            [r,c] = size(time);

            mappa = NaN(size(time));

            Tmin=mean2(obj.temp(:,:,1));

            for ii = 1:r
                for jj = 1:c

                    T_cut=obj.temp(ii,jj,t1:t2)-Tmin;
                    %Normalizza pixel per pixel
                    T_norm=squeeze(T_cut./max(T_cut,[],3));

                    %trova i punti di meta riscaldamento
                    Area = sum(T_norm);

                    mappa(ii,jj)= Area;
                end
            end
            surf(mappa,EdgeColor='none')
            colorbar
            colormap("hot")
            view (2)
        end


        function  LockInAmplifier(obj,f, a, b)

            [r,c,s]=size(obj.temp(:,:,a:b));
            nfft=s; %no 0 padding è meglio
            if isempty(obj.framerate)
                obj.framerate = obj.metadata.FrameRate;
            end
            %             FFT=fft(obj.temp,nfft,3);
            obj.f_c2 = obj.framerate*(0:(nfft/2-1))/nfft; % x-axis in Hz

            t=obj.time(a:b)';



            COS=2*cos(f*2*pi()*t);
            SIN=2*sin(f*2*pi()*t);

            for i=1:r
                for j=1:c


                    F=squeeze(obj.temp(i,j,a:b)-mean(obj.temp(i,j,a:b))).*COS;

                    G=squeeze(obj.temp(i,j,a:b)-mean(obj.temp(i,j,a:b))).*SIN;


                    X=mean(F);
                    Y=mean(G);

                    A(i,j)=sqrt(X^2+Y^2);
                    P(i,j)=atan2(Y,X);

                end
            end



            %             A= abs(FFT(:,:,1:nfft/2)/nfft);
            %             A(:,:,2:end-1)=2*A(:,:,2:end-1);

            %             P=  angle (FFT(:,:,1:nfft/2));

            obj.A=A;
            obj.P=P;

            %crea le mappe a f=freq


        end

        function  LockIn(obj)

            [r,c,s]=size(obj.temp);
            nfft=s; %no 0 padding è meglio
            if isempty(obj.framerate)
                obj.framerate = obj.metadata.FrameRate;
            end
            FFT=fft(obj.temp,nfft,3);
            obj.f_c2 = obj.framerate*(0:(nfft/2-1))/nfft; % x-axis in Hz



            A= abs(FFT(:,:,1:nfft/2)/nfft);
            A(:,:,2:end-1)=2*A(:,:,2:end-1);

            P=  angle (FFT(:,:,1:nfft/2));

            obj.A=A;
            obj.P=P;

            %crea le mappe a f=freq


        end


        function  [Cut_x,yp,Cut_y,xp,xc,yc]=LockinAmplifierResults(obj,freq,mmpxratio,tol, save)

            if ~exist("save", "var")
                save = 0;
            end

            A=obj.A;
            PLru=obj.P;


            PLru=PLru;





            PLru(A < tol) = nan;
            A(A<tol)=nan;
            PLru=unwrap(PLru);
            %
            % A=A(2:end,:);
            %  PLru=PLru(2:end,:);
            %
            % A(abs(diff(A,2,1))>tol2 & abs(diff(PLru,2,1))>tol2)=nan;
            % A=A(:,2:end);
            % A(abs(diff(A,2,2))>tol2)=nan;

            %
            %
            % PLru(diff(PLru,2,1)<tol2)=nan;
            % PLru=PLru(:,2:end);
            % PLru(diff(PLru,2,2)<tol2)=nan;


            %manual unwrap

            %             wraptol=1;
            [r,c]=size(PLru);
            X=mmpxratio*[1:c];
            Y=mmpxratio*[1:r];
            % %su asse x
            % D=diff(PLru,1,2);
            %             for ii=1:r
            %                 for jj=2:c
            %                     d=PLru(ii,jj)-PLru(ii,jj-1);
            %                     if abs(d)>wraptol
            %                         PLru(ii,jj:end)=PLru(ii,jj:end)-d;
            %                     end
            %                 end
            %             end

            % %poi su y

            % D=diff(PLru,1,1);
            %             for ii=2:r
            %                 for jj=1:c
            %                     if abs(D(ii-1,jj))>wraptol
            %                         PLru(ii:end,jj)=PLru(ii:end,jj)-D(ii-1,jj);
            %                     end
            %                 end
            %             end
            %
            %        figure
            %  surf(D,'EdgeColor','none')
            %  D=diff(PLru,1,1);
            %  figure
            %  surf(D,'EdgeColor','none')


            % MAPPE
            f = figure;




            PLruSurf = PLru-min(PLru, [], "all");
            PLru = PLru-min(PLru, [], "all");
            M = zeros(size(PLru));
            M(~isnan(PLru)) = 1;
            app = sum(M,1);
            xc = floor(mean(find(app >= max(app)-3)));
            app = sum(M,2);
            yc = floor(mean(find(app >= max(app)-3)));
            surf(X,Y,PLruSurf,'EdgeColor','none')
            view([0,-90])
            axis equal
            xlabel('X')
            ylabel('Y')
            title(string(['Phase Map at ',num2str(freq),'Hz']))
            if save > 0
                savefig(f, fullfile(obj.saveDir, 'phase_map.fig'));
                close(f)
            end

%             [Gmag,Gdir] = imgradient(PLruSurf)
%             figure
%             surf(X,Y,Gmag,'EdgeColor','none')
%             view([0,-90])
%             axis equal
%             xlabel('X')
%             ylabel('Y')
%             title(string(['Magnitude of the Gradient for the Phase Map at ',num2str(freq),'Hz']))
% 
%             figure
%             surf(X,Y,Gdir,'EdgeColor','none')
%             view([0,-90])
%             axis equal
%             xlabel('X')
%             ylabel('Y')
%             title(string(['Direction of the Gradient for the Phase Map at ',num2str(freq),'Hz']))

            [r,c]=size(A);
            X=mmpxratio*[1:c];
            Y=mmpxratio*[1:r];

            f = figure;
            surf(X,Y,A,'EdgeColor','none')
            view([0,-90])
            axis equal
            xlabel('X')
            ylabel('Y')
            title(string(['Amplitude Map at ',num2str(freq),'Hz']))

            if save > 0
                savefig(f, fullfile(obj.saveDir, 'amp_map.fig'));
                close(f)
            end


            % Phase Cut

            f = figure;
            subplot(2,1,1)
            Cut_y=PLru(:,xc);
            lx=length(Cut_y);
            xp=mmpxratio*[1:lx];

            plot(xp,Cut_y)
            title('Phase along y axis')
            xlabel('y[mm]')
            ylabel('Phase [rad]')

            subplot(2,1,2)
            Cut_x=PLru(yc,:);
            ly=length(Cut_x);
            yp=mmpxratio*[1:ly];

            plot(yp,Cut_x)
            title('Phase along x axis')
            xlabel('x[mm]')
            ylabel('Phase [rad]')
            if save > 0
                savefig(f, fullfile(obj.saveDir, 'phase_cut.fig'));
                close(f)
            end

        end



        function  LockinResults(obj,freq,xc,yc,mmpxratio,tol)
            A=obj.A;
            P=obj.P;
            f_c2=obj.f_c2;
            figure
            semilogy(f_c2,squeeze(A(yc,xc,:)))
            title('FRF in xc-yc')

            [~,index]=min(abs(f_c2-freq));

            PLru=(P(:,:,index));



            %P_sound=mean(PLru(y_sound-span_sound:y_sound+span_sound,x_sound-span_sound:x_sound+span_sound),"all");


            %             PLru(A(:,:,index) < tol) = min(P,[],'all');
            PLru(A(:,:,index) < tol) = nan;


            PLru=unwrap(PLru);
            %manual unwrap

            %             wraptol=1;
            % [r,c]=size(PLru);

            % %su asse x
            % D=diff(PLru,1,2);
            %             for ii=1:r
            %                 for jj=2:c
            %                     d=PLru(ii,jj)-PLru(ii,jj-1);
            %                     if abs(d)>wraptol
            %                         PLru(ii,jj:end)=PLru(ii,jj:end)-d;
            %                     end
            %                 end
            %             end

            % %poi su y

            % D=diff(PLru,1,1);
            %             for ii=2:r
            %                 for jj=1:c
            %                     if abs(D(ii-1,jj))>wraptol
            %                         PLru(ii:end,jj)=PLru(ii:end,jj)-D(ii-1,jj);
            %                     end
            %                 end
            %             end
            %
            %        figure
            %  surf(D,'EdgeColor','none')
            %  D=diff(PLru,1,1);
            %  figure
            %  surf(D,'EdgeColor','none')


            % MAPPE
            figure
            surf(PLru,'EdgeColor','none')
            view([0,-90])
            axis equal
            xlabel('X')
            ylabel('Y')
            title(string(['Phase Map at ',num2str(freq),'Hz']))


            figure
            %             surf(A(:,:,index),'EdgeColor','none')
            surf(A(:,:,index),'EdgeColor','none')
            view([0,-90])
            axis equal
            xlabel('X')
            ylabel('Y')
            title(string(['Amplitude Map at ',num2str(freq),'Hz']))

            % Phase Cut

            figure
            subplot(2,1,1)
            Cut_y=PLru(:,xc);
            lx=length(Cut_y);
            xp=mmpxratio*[1:lx];

            plot(xp,Cut_y)
            title('Phase along y axis')
            xlabel('y[mm]')
            ylabel('Phase [rad]')

            subplot(2,1,2)
            Cut_x=PLru(yc,:);
            ly=length(Cut_x);
            yp=mmpxratio*[1:ly];

            plot(yp,Cut_x)
            title('Phase along x axis')
            xlabel('x[mm]')
            ylabel('Phase [rad]')

        end





        function mappa = evalHeating(obj, samples)
            %evalHeating valuta il tempo in cui si raggiunge la metà della
            %curva di riscaldamento. Il tempo si riferisce all'instante dal
            %quale è iniziato il riscaldamento per il singolo pixel
            %   samples è il numero di campioni iniziali in cui sicuramente
            %   il laser era spento (in campioni, non secondi!)
            %   mappa è la mappa in output, calcolata sulle temperature
            %   correnti (al netto di filtri e cose varie)

            [maxT, time] = obj.getMaxTemp(); % prendo l'istante di tempo in cui la temperatura raggiunge il massimo (pixel per pixel)

            [r,c] = size(time);

            mappa = NaN(size(time));

            for ii = 1:r
                for jj = 1:c
                    if ~isnan(maxT(ii,jj))
                        % cerco quando comincia il riscaldamento. valuto la
                        % temperatura iniziale e lo scarto quadratico medio
                        T1 = mean(obj.temp(ii,jj,1:samples));
                        sq1 = sqrt(var(obj.temp(ii,jj,1:samples)));

                        % cerco il primo istante in cui la temperatura
                        % supera la media + 3 scarto
                        tIni = find(obj.temp(ii,jj,:)>=T1+3*sq1, 1 );

                        % Fissato il tempo in cui misuro la temperatura
                        % massimo tMax, cerco la temperatura massima come
                        % media delle temperature nel tempo
                        % [tMax-10, tMax+30] (in campioni). Nota che quando
                        % fai l'acquisizione "impulsiva", la frequenza di
                        % campionamento è circa 750 Hz, quindi 40 campioni
                        % sono la media su circa 0.05 secondi

                        T2 = mean(obj.temp(ii,jj,time(ii,jj)-10:time(ii,jj)+40));

                        % A questo punto cerco il primo istante in cui la
                        % temperatura attraversa (T2-T1)/2.

                        tMezzi = find(obj.temp(ii,jj,:)>=(T2+T1)/2, 1 );

                        % Salvo nella mappa il tempo (tMezzi - tIni) in
                        % secondi

                        mappa(ii,jj) = (tMezzi-tIni)/obj.metadata.FrameRate;
                    end
                end
            end
        end

        function [b,a] = creaFiltro(obj, fc, order)
            fs = obj.metadata.FrameRate;

            [b,a] = butter(order,fc/(fs/2));
        end

        function filtroSpaziale(obj, s)
            %filtroSpaziale applica un filtro spaziale su ogni frame della
            %radiance. La temperatura va ricalcolata a posteriori con la
            %funzione di normalizzazione
            %   size è la dimensione del filtro
            d = size(obj.radiance);
            for ii = 1:d(3)
                obj.radiance(:,:,ii) = wiener2(obj.radiance(:,:,ii), [s s]);
            end
        end

        function filtroTemporale(obj, b, a)
            %filtroTemporale applica il filtro temporale ad ogni pixel
            %della radiance. La temperatura va ricalcolata a posteriore con
            %la funzione di normalizzazione
            %   b e a sono i parametri del filtro da calcolare con
            %   creaFiltro

            d = size(obj.radiance);

            for ii = 1:d(1)
                for jj = 1:d(2)
                    obj.radiance(ii,jj,:) = filtfilt(b,a,squeeze(obj.radiance(ii,jj,:)));
                end
            end
        end

        function metadata = getMetadata(obj)
            metadata = obj.metadata;
        end


        function Tmax= TMaxVsTime(obj)
            figure

            [temp, time] = max(obj.temp, [], 3);
            M=max(max(max(obj.temp)));

            [row,col]=find(temp==M,1);
            Tmax=(squeeze(obj.temp(row,col,:)));
            plot(obj.time,Tmax)

        end


        function  GetSpotSizeByFirstMax(obj,frameMax,tol,mmpxratio)
            S=obj.temp(:,:,frameMax);
            S=S-min(S,[],'all');
            S=S/max(S,[],'all');
            S(S<tol)=nan;
            [r,c]=size(S);
            X=mmpxratio*[1:c];
            Y=mmpxratio*[1:r];
            surf(X,Y,S,'edgecolor','none')


        end





        function Tmax= TMaxVsFrame(obj)
            figure

            [temp, time] = max(obj.temp, [], 3);
            M=max(max(max(obj.temp)));

            [row,col]=find(temp==M,1);
            Tmax=(squeeze(obj.temp(row,col,:)));
            plot(Tmax)

        end

        function Tensore_denoised = SVDdenoising(obj)
            frame=size(obj.temp,3);
            px=size(obj.temp,1);
            py=size(obj.temp,2);
            Matrice=reshape(obj.temp,frame,[]);

            Results = svde(Matrice);
            Matrice_denoised = Results.AR;


            obj.temp=reshape(Matrice_denoised(:,:),px,py,frame);
            Tensore_denoised= obj.temp;
        end

        function SelectTimeInterval(obj,frame_start,frame_end)
            obj.temp=obj.temp(:,:,frame_start:frame_end);
            obj.time=obj.time(frame_start:frame_end);
            obj.radiance=obj.radiance(:,:,frame_start:frame_end);
        end

        function  [Dx,Dy,Davg]=evaluateDiffusivity(obj,freq,xc,yc,mmpxratio,laserspotdiameter,expectedDiffusivity,tol)
            if size(obj.P,3)>1
                [~,index]=min(abs(obj.f_c2-freq));
                P=obj.P(:,:,index);A=obj.A(:,:,index);
            else


                P=obj.P;A=obj.A;
            end


            P(A< tol) = nan;

            xstart=xc+round(2*laserspotdiameter/2/mmpxratio);
            expectedThermalDiffusionLength=sqrt(expectedDiffusivity/freq/pi());
            xend=xstart+round(2*expectedThermalDiffusionLength/mmpxratio);

            yend=yc-round(2*laserspotdiameter/2/mmpxratio);
            ystart=yend-round(2*expectedThermalDiffusionLength/mmpxratio);

            slopey=unwrap(P(ystart:yend,xc));
            slopex=unwrap(P(yc,xstart:xend));


            xp=mmpxratio*[ystart:yend];


            yp=mmpxratio*[xstart:xend];


            [xData, yData] = prepareCurveData( yp, slopex );

            % Set up fittype and options.
            ft = fittype( 'poly1' );
            opts = fitoptions( 'Method', 'LinearLeastSquares' );

            % Fit model to data.
            [fitresult, gof] = fit( xData, yData, ft, opts )

            % Plot fit with data.
            figure( 'Name', 'Fit along x' );
            h = plot( fitresult, xData, yData );
            % Label axes
            xlabel( 'x', 'Interpreter', 'none' );
            ylabel( 'Phase x', 'Interpreter', 'none' );
            grid on


            mx=fitresult.p1;
            R2x=gof.rsquare;

            [xData, yData] = prepareCurveData( xp, slopey );

            % Set up fittype and options.
            ft = fittype( 'poly1' );
            opts = fitoptions( 'Method', 'LinearLeastSquares' );

            % Fit model to data.
            [fitresult, gof] = fit( xData, yData, ft, opts )

            % Plot fit with data.
            figure( 'Name', 'Fit along y' );
            h = plot( fitresult, xData, yData );
            % Label axes
            xlabel( 'y', 'Interpreter', 'none' );
            ylabel( 'Phase y', 'Interpreter', 'none' );
            grid on


            my=fitresult.p1;
            R2y=gof.rsquare;


            Dx=freq*pi()/mx^2;
            Dy=freq*pi()/my^2;
            Davg=(Dx+Dy)/2;

        end

        function  surf(obj)
            frame=size(obj.temp,3);

            for i=1:frame

                figure(20)
                surf(obj.temp(:,:,i),'EdgeColor','none')
                title(string(i))
                view(2)
                colormap("hsv")
                caxis([min(obj.temp,[],'all'),max(obj.temp,[],'all')])
                colorbar



                drawnow()
            end
        end
    end



end
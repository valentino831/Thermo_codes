%%  SVD de-noising

%--------------------------------------------------------------------------
% svde.m
% Dr. Brenden Epps
% 23-September-2019
% 
% Singular Value Decomposition (SVD) with error analysis.
% 
% The SVD of matrix M is:  M = U*diag(S)*V',  where U and V are unitary  
% matrices containing the singular vectors, and S is an array containing 
% the singular values.  See Matlab function svd().
%
% Here, the premise is that one wants to perform the SVD of matrix M, 
% but M is corrupted by additive noise as follows:  
%
%       M = A + E,   M = noisy data matrix  
%                    A = clean data matrix  
%                    E = random noise matrix
%
% Assume that the size of M, A, and E are all [T,D], and conceptualize T as 
% the number of timesteps and D as the number of data sites.
%
% Let epsilon be the root mean square error in the noisy data:
%       sqrt( sum( (M(:)-A(:)).^2 ) / (T*D) ) = epsilon
%
% Assume the error data E are assumed to be drawn from a normal distribution 
% with zero mean and standard deviation epsilon. The E data may be independent 
% or may be spatially-correlated (i.e. correlated across rows of E).
%
% Given noisy data M but unknown A or epsilon, svde.m can be used to perform 
% the SVD of M and the following error analysis:
%   -- esitmate the error level, epsilon, in the given data M.
%   -- estimate the "error bars" (95% confidence intervals) for the SVD results 
%      (i.e. expected value and  standard deviation of the singular values and vectors)
%   -- estimate the overall rms error of the SVD modes (singular vectors), and
%   -- filter the noise in M to form a reconstruction, AR, of the clean data 
%      that is as most accurate to A as possible.
%
%--------------------------------------------------------------------------
% Function call:
%   d = svde(M,varargin)
%
% Inputs:
%   M   = size [T,D] matrix of noisy "measured" data, where T < D.  
%         For sake of semantics, assume each row of M is a snapshot in time,
%         and each column of M is a data site.
%
%   varargin = optional list of sting flags:
%     'RemoveTimeAvg'   == remove time average (average over each column) before performing SVD.
%     'UVstd'           == compute the expected value (average) and standard deviation of the noisy singular vectors (computationally-expensive)
%     'w',<value>       == use specified <value> of w, the 'effective smoothing window width'.
%     'f',<value>       == use specified <value> of f, the 'spatial-correlation factor'.  If both w and f are provided, w is used, and f is ignored.
%     'epsilon',<value> == use specified <value> of epsilon, the rms error between the noisy and clean data. 
%
%   For usage, see examples below.
%
% Outputs:
%  d  = data structure with the following outputs
%
%     options = varargin = cell array of optional input flags
%
%     M = given noisy "measured" data, size [T,D]
%
%     U = left  singular vectors,      size [T,K] 
%     S = singular values,             size [K,1]
%     V = right singular vectors,      size [D,K]
% 
%     epsilon  = assumed rms error between noisy data M and clean data A.  epsilon is the value used in the SVD error calculations.
%     epsilon0 = best estimate of epsilon.  Nominally epsilon = epsilon0 unless epsilon is specified using the 'epsilon' flag.
%     epsilon1 = error level for best-fit Marchenko-Pastur distribution fit to tail of singular values S.
%     f   = spatial-correlation factor       (f = 1 for i.i.d. error, and f > 1 for spatially-correlated error)
%     w   = effective smoothing window width (w = 1 for i.i.d. error, and w > 1 for spatially-correlated error)
%     SE1 = best fit Marchenko-Pastur distribution fit to tail of S, where SE1 = epsilon1*(unit M-P distribution), size [K,1]
%
%     KF  = index such that  S(KF) < epsilon* sqrt(T*D)  
%     K2  = index such that  S(K2) < epsilon*(sqrt(f*T)+sqrt(D))
%     KE  = index such that  S(KE) < epsilon* sqrt(D)           
% 
%     Sgap    = gap between neighboring singular values, size [K,1]
%
%     Savg,Sstd = expected value and standard deviation (of many Monte-Carlo trials) of the noisy singular values     , size [1,K]
%     Lavg,Lstd = expected value and standard deviation (of many Monte-Carlo trials) of the noisy eigenvalues, L = S^2, size [1,K]
% 
%     Urmse = expected root mean square error between noisy and clean left  singular vectors U, size [1,K]
%     Vrmse = expected root mean square error between noisy and clean right singular vectors V, size [1,K]
%     Usin  = sine of the canonical angle     between noisy and clean left  singular vectors U, size [1,K]
%     Vsin  = sine of the canonical angle     between noisy and clean left  singular vectors V, size [1,K]
%
%     SR      = reconstructed estimate of the clean singular values, size [K,1]
%     AR      = reconstructed estimate of the clean data using rank r=r_min, size [T,D] 
%
%     loss    = estimate of LOSS(AR_r,A) for rank r = 1:K, i.e. size(loss) == [1,K], where
%                                AR_r = rank r reconstruction of the clean data
%
%               If the clean data A were known, then the "LOSS" (measure of accuracy) of 
%               reconstruction AR_r could be computed by the following definition:
%                  
%                 LOSS(AR_r,A) = sum( (AR_r(:)-A(:)).^2 ) = square of the Frobenius norm of (AR_r - A)
%                  
%               Since A is unknown, so loss(r) is a theoretical estimate of LOSS(AR_r,A). 
%
%     Vrmse1  = Vrmse computed using w=1. Vrmse1 is used to compute r_min, size [1,K]
%     r_min   = rank for which LOSS(AR_r,A) is expected to be a minimum.
%               Note that since loss(r) is only an estimate of the true LOSS(AR_r,A),
%               r_min might not necessarily be the index for which "loss" is a minimum.
%     r_alt   = alternative rank (see reference (Epps and Krivitzky 2019b))
%
%      Mloss  = estimate of the LOSS of the original data M,  Mloss = epsilon^2*D*T   
%     ARloss  = estimate of the LOSS of reconstruction AR,   ARloss = loss(r_min)
%     ARmerit = figure of merit for the reconstruction AR.
%
%                ARmerit = 1 - ARloss/Mloss
%
%                If ARmerit = 0, then AR is no more accurate (no smaller loss) than the original data M.
%                If ARmerit = 1, then AR = A is a perfect reconstruction. i.e. 100% of the noise was removed.
%
%  optional outputs:
%    'RemoveTimeAvg'  triggers the code to remove the time-average data 
%                     (average over each column) before performing the SVD.
%                     
%       tavgM = time-averaged M, repeated in each row, size [T,D]
%       M0  = M  - tavgM = given input data with time-average removed, size [T,D] 
%       AR0 = AR - tavgM = reconstructed estimate of the clean data, with time-average removed
%         
%           If 'RemoveTimeAvg', then the SVD is computed on M0 as in M0 = U*diag(S)*V', 
%           AR0 is the computed reconstruction estimate, and AR = AR0 + tavgM.
%
%    'UVstd'
%       Uavg,Vavg = expected value (average) of the noisy singular vectors
%       Ustd,Vstd = standard deviation       of the noisy singular vectors 
%
%--------------------------------------------------------------------------
% SVD "error bars" (95% confidence intervals):
%   The above quantities (Savg,Uavg,Vavg) and (Sstd,Ustd,Vstd) are estimates of the 
%   averages and standard deviations that would be obtained from many Monte Carlo trials.  
%
%   The "error bars" for the singular values  can be plotted as:     S +/- 1.96*Sstd
%   The "error bars" for the singular vectors can be plotted as:  Uavg +/- 1.96*Ustd
%                                                                 Vavg +/- 1.96*Vstd
%--------------------------------------------------------------------------
% Example function calls:
%   d = svde(M)  computes the SVD of matrix M, estimates the error in M, 
%                estimates the error in the SVD of M, filters the noise 
%                from M, and outputs the results in data structure d.
%
%   d = svde(M,'RemoveTimeAvg')  replaces M with M minus the average over 
%               each column, and then performs the above calculations.
%
%   d = svde(M,'w',VALUE) estimates the error in M assuming w = VALUE.
%
%   d = svde(M,'f',VALUE) estimates the error in M assuming f = VALUE.
%               If 'f' and 'w' are both specified, 'w' is used, and 'f' is ignored.
%
%   d = svde(M,'epsilon',VALUE) estimates the error in the SVD assuming epsilon = VALUE.
%
%   d = svde(M,'UVstd')   compute the standard deviation of the mode shapes.
%                This calculation is optional, because it takes a long time.
%
%  Option flags can be given in any order:  
%   d = svde(M,'RemoveTimeAvg','w',VALUE1,'epsilon',VALUE2,'UVstd') yields the same result as
%   d = svde(M,'epsilon',VALUE2,'UVstd','RemoveTimeAvg','w',VALUE1)
%
%--------------------------------------------------------------------------
% Reference:
% 1. Brenden Epps and Eric Krivitzky (2019a) "Singular value decomposition of noisy data: mode corruption", Experiments in Fluids 60:121, https://doi.org/10.1007/s00348-019-2761-y
% 2. Brenden Epps and Eric Krivitzky (2019b) "Singular value decomposition of noisy data: noise filtering", Experiments in Fluids 60:126, https://doi.org/10.1007/s00348-019-2768-4
%--------------------------------------------------------------------------

function d = svde(M,varargin)

[T,D] = size(M);   % T = number of timesteps, D = number of data sites

if T >= D
    error('Input data must have more columns than rows')
else
    K = T;  % K = number of modes
end


% Possibly remove time-average data from each data site
if any( strcmp(varargin, 'RemoveTimeAvg') )
    
    tavgM = repmat( mean(M,1), T,1); % [T,D] time-average, repeated in each row

    M = M - tavgM;                   % [T,D] remove time-average
    
end
    
%--------------------------------------------------------------------------
% Peform singular value decomposition:  M = U*Smat*V'
%   size(U)    == [T,K],       U(:,k) = left  singular vector k
%   size(Smat) == [T,K],    Smat(k,k) =       singular value  k
%   size(V)    == [D,K],       V(:,k) = right singular vector k
[U,Smat,V] = svd(M,'econ');  

S = diag(Smat); % [T,1] column vector of singular values


%--------------------------------------------------------------------------
% MIGHT NEED TO ADD SOME SORT OF ERROR CHECKING HERE FOR ZERO SINGULAR VALUES IN ORDER TO AVOID PROBLEMS WITH THEORY.

% % % % The last singular value is zero, SMmat(T,T) == 0, so get rid of it to avoid problems.
% % % % Also, get rid of first timestep in M so that times in M match those in A.
% % % T  = T-1;   K = T;
% % % Smat = Smat(1:T,1:T);
% % % S =         S(1:T);
% % % V =       V(1:D,1:T);
% % % U =     U(2:T+1,1:T);   % retain timesteps [2:T+1] <--> [3:2:T0] to match up with A, but modes 1:T to get rid of the last mode (now T+1).
%--------------------------------------------------------------------------


%--------------------------------------------------------------------------
% Noise Level Estimation (epsilon,epsilon1,SE1,f,w)
%--------------------------------------------------------------------------
if any( strcmp(varargin, 'w') )  % smoothing window width, w, is specified
    
    i = find(strcmp(varargin, 'w'));
    
    w = cell2mat(varargin(i+1));  % specified smoothing window width

    % Determine spatial correlation factor, f, by solution of implicit equation:  w = 1 + (2*f-1.5)*(1-exp(-20*(f-1)))
    % For f >> 1, this reduces to w = 1 + (2*f-1.5) = 2*f - 0.5
    if w >= 4
        
        f = w/2 + 0.25;  % this is within 1e-11 of the f computed via interp1(...) below
        
        %  w_check = 1 + (2*f-1.5)*(1-exp(-20*(f-1)))
    else  % w < 4
        f_est = max(w/2 + 0.25, 1);  % ensure f >= 1
        f_all = f_est : 0.001  : (f_est+0.1);   % f will always be greater than f_est, so only need to check higher values

        w_all = 1 + (2*f_all-1.5).*(1-exp(-20*(f_all-1)));

        f = interp1( w_all,f_all, w);

        %     w_check = 1 + (2*f-1.5)*(1-exp(-20*(f-1)))
        %     f_est
        %     f
        %     f - f_est
    end

    [epsilon0, epsilon1, SE1] = SVD_Estimate_Error_v7(S,T,D,f);
    
elseif any( strcmp(varargin, 'f') )  % spatial correlation factor, f, is specified
    
    i = find(strcmp(varargin, 'f'));
    
    f = cell2mat(varargin(i+1));  % specified spatial correlation factor
    
    [epsilon0, epsilon1, SE1] = SVD_Estimate_Error_v7(S,T,D,f);
    
    w = 1 + (2*f-1.5)*(1-exp(-20*(f-1)));  % "effective smoothing window width"
    
else % neither w nor f is specified...determine f within SVD_Estimate_Error_v7.m
  
    [epsilon0, epsilon1, SE1, f] = SVD_Estimate_Error_v7(S,T,D,NaN);

    w = 1 + (2*f-1.5)*(1-exp(-20*(f-1)));  % "effective smoothing window width"    
    
end

% Possibly override epsilon with specified error level
if any( strcmp(varargin, 'epsilon') )  % error level, epsilon, is specified
    
    i = find(strcmp(varargin, 'epsilon'));
    
    epsilon = cell2mat(varargin(i+1));  % specified error level
else
    epsilon = epsilon0;  % use value determined above
end


% Indices of interest:
KF = find( S < epsilon* sqrt(T*D)           , 1,'first');  
K2 = find( S < epsilon*(sqrt(f*T)+sqrt(D))  , 1,'first');
KE = find( S < epsilon* sqrt(D)             , 1,'first');


% Gap between singular values:
Sgap = SVD_ComputeGap_v2(S);  % size [K,1]

%--------------------------------------------------------------------------
% SVD Perturbation Analysis
%--------------------------------------------------------------------------
% Compute expected value (average) and standard deviation of 
% several quantities related to the singular value decomposition
% of noisy data matrix M using the perturbation theory detailed in
% (Epps and Krivitzky 2019a,b).
[Lavg,Lstd, Savg,Sstd, Uavg, Vavg, Urmse,Vrmse, Usin,Vsin] = SVD_Perturbation_Theory_v6(U,S,V,epsilon,w);


% Possibly compute standard deviation of mode shapes (calculation optional, because it takes a long time)
if any( strcmp(varargin, 'UVstd') )
    disp('Computing Ustd and Vstd...please be patient...')
    
    [Ustd,Vstd] = SVD_Perturbation_Theory_UVstd_v6(U,S,V,epsilon,w);
    
    disp('Done computing Ustd and Vstd.')
end


%--------------------------------------------------------------------------
% Noise Filtering:
%--------------------------------------------------------------------------
[~,Vrmse1] = SVD_Perturbation_Theory_RMSE_v6(S,epsilon,T,D,1);   % rmse with w=1

t0   = ( log(Vrmse ) - log(sqrt(2/D)) )/ ( log(Vrmse(1) ) - log(sqrt(2/D)) );  % using w
t1   = ( log(Vrmse1) - log(sqrt(2/D)) )/ ( log(Vrmse1(1)) - log(sqrt(2/D)) );  % using w = 1

r_alt = find( t0 > 0.05, 1,'last');
r_min = find( t1 > 0.05, 1,'last');  % optimal rank for minimum-loss noise filtering

% Reconstruct the singular values ('E15' method):
SR       = sqrt( S.^2 - SE1.^2 );
   index = find( S.^2 - SE1.^2 < 0, 1);
SR(index:K) = 0;


% Filtered data matrix (using SVD modes up to rank r_min)
AR = U(:,1:r_min) * diag( SR(1:r_min) ) * V(:,1:r_min)';  


% Estimate of loss betwen filtered data AR and clean data A
SRsquared = SR.^2;  % size [K,1]

loss = zeros(1,K);

for r = 1:K
    loss(r) = sum( SRsquared(r+1:K) ) + epsilon^2*D*r;  
end

 Mloss  = epsilon^2*D*T;     % estimate of LOSS for original data M
ARloss  = loss(r_min);       % estimate of LOSS for reconstruction AR  (i.e. loss at rank r_min)
ARmerit = 1 - ARloss/Mloss;  % figure of merit  for reconstruction AR

%--------------------------------------------------------------------------
% Pack up output data structure:
d.options = varargin;  % if no varargin options are given, then options = {}

if any( strcmp(varargin, 'RemoveTimeAvg') )
    d.M0    = M;          % original input data, with time-average removed
    d.M     = M + tavgM;  % original input data M
    d.tavgM =     tavgM;  % time-average data
else
    d.M     = M;          % original input data M
end

d.U = U;
d.S = S;
d.V = V;

d.text1 = '------------------------';

d.epsilon  = epsilon;
d.epsilon0 = epsilon0;
d.epsilon1 = epsilon1;

d.f = f;
d.w = w;

d.SE1 = SE1;

d.KF = KF;
d.K2 = K2;
d.KE = KE;

d.Sgap = Sgap;

d.text2 = '------------------------';
d.Savg = Savg;
d.Sstd = Sstd;

d.Lavg = Lavg;
d.Lstd = Lstd;

d.Urmse = Urmse;
d.Vrmse = Vrmse;

d.Usin  = Usin;
d.Vsin  = Vsin;

if any( strcmp(varargin, 'UVstd') )
    
    d.Uavg = Uavg;
    d.Ustd = Ustd;
    
    d.Vavg = Vavg;    
    d.Vstd = Vstd;
end


d.text3 = '------------------------';
if any( strcmp(varargin, 'RemoveTimeAvg') )
    
    d.AR0    = AR;          % filtered data, excluding time average
    d.AR     = AR + tavgM;  % filtered data, including time average
else
    d.AR     = AR;          % filtered data, including time average
end
d.SR         = SR;
d.loss       = loss;
d.Vrmse1     = Vrmse1;
d.r_min      = r_min;
d.r_alt      = r_alt;
d.Mloss      =  Mloss;
d.ARloss     = ARloss;
d.ARmerit    = ARmerit;

%--------------------------------------------------------------------------
end % function svde
%--------------------------------------------------------------------------




%--------------------------------------------------------------------------
% SVD_Perturbation_Theory_v6.m
% Dr. Brenden Epps
% 23-Sep-2019
%
% This function computes the expected value and standard deviation of 
% several quantities related to the singular value decomposition
% of noisy data matrix M, using the perturbation theory detailed in
% (Epps and Krivitzky 2019a,b).
%
% Inputs:
%   U,S,V   = SVD of clean data matrix A = U*S*V'
%             -- S may be a matrix or an array of singular values
%             -- If {U,S,V} unknown, can use noisy {UM,SM,VM} from noisy data M = UM*SM*VM'
%
%   epsilon = rms error between the noisy data M and the clean data A
%
%   w       = effective smoothing window width (w=1 for i.i.d. noise) (see references).
%
% Outputs:
%   Lavg, Lstd      eigenvalues, L=S^2,     expected value (average over many Monte-Carlo trials) and standard deviation (over many Monte-Carlo trials)
%   Savg, Sstd      singular values,        expected value (average over many Monte-Carlo trials) and standard deviation (over many Monte-Carlo trials)
%   Uavg,           left  singular vectors, expected value (average over many Monte-Carlo trials)
%   Vavg,           right singular vectors, expected value (average over many Monte-Carlo trials)
%   Urmse,Vrmse     root mean square error, expected value (average over many Monte-Carlo trials)
%   Usin ,Vsin      sin( canonical angle ), expected value (average over many Monte-Carlo trials)
%   Ucos ,Vcos      cos( canonical angle ), expected value (average over many Monte-Carlo trials)
%--------------------------------------------------------------------------
% Version notes:
% v2  calculations vectorized but no changes to the theory.
%
% v3  introduced theory for spatially-correlated data, via "effective smoothing window width", 'w'.
%     used w for modes 1:KF-1 then w=1 for modes KF:K,  where KF = find( S < epsilon*sqrt(T*D), 1,'first')
%
% v4  uses w for all modes.  final version used in (Epps and Krivitzky 2019a,b) papers.
%
% v5  same algorithm as v4 for all quantities except Uavg,Vavg.  also, other code cleanup.
%     fixed sign error in Vavg (which did not affect results in either (E & K 2019a,b) paper, as documented below). 
%       [Urmse,Vrmse, Usin,Vsin, Ucos,Vcos] identical code to that in SVD_Perturbation_Theory_RMSE_v5.m
%       [Ustd, Vstd]  moved from this code into SVD_Perturbation_Theory_UVstd_v6.m (because calculations are slow).
%       [Ucos ,Vcos]  first introduced in this version.
%
% v6  release version.  same algorithm as v5, but with code cleanup.
%
%--------------------------------------------------------------------------
% Reference:
% 1. Brenden Epps and Eric Krivitzky (2019a) "Singular value decomposition of noisy data: mode corruption", Experiments in Fluids 60:121, https://doi.org/10.1007/s00348-019-2761-y
% 2. Brenden Epps and Eric Krivitzky (2019b) "Singular value decomposition of noisy data: noise filtering", Experiments in Fluids 60:126, https://doi.org/10.1007/s00348-019-2768-4
%--------------------------------------------------------------------------

function [Lavg,Lstd, Savg,Sstd, Uavg, Vavg, Urmse,Vrmse, Usin,Vsin, Ucos,Vcos] = SVD_Perturbation_Theory_v6(U,S,V,epsilon,w)

if nargin < 5
    w = 1;  % value for i.i.d. noise
end

[N1,N2] = size(S);

if N1 > 1 && N2 > 1 % S is a matrix
    S = diag(S);    % S is now a vector
end

T = size(U,1);
D = size(V,1);
K = length(S);

if T > D
    error('SVD_Perturbation_Theory.m is only coded for T < D case!')
elseif K ~= T
    error('number of singular values inconsistent with T = size(U,1).')
end

S = S(:)';  % S is now a row vector of size (1,K)
L = S.^2;   

%--------------------------------------------------------------------------
Lstd = epsilon*sqrt(w) *  2.*S;     % size [1,K]
Sstd = epsilon*sqrt(w) * ones(1,K); % size [1,K]


% In order to speed up the code, vectorize all calculations, and 
% perform summations over one matrix index. Define m and k as matrices.  
% Perform sum( ..., 1) to sum over m down each column and leave a row array  
% indexed by k.
m = zeros(K-1,K);

for k = 1:K
    m(:,k) = setdiff( [1:K]', k );  % m changes down each column
end

k = repmat([1:K], K-1,1);  % k is constant down each column and changes across each row

temp = sum( (L(m)+L(k))./(L(m)-L(k)), 1 );    % sum(...,1) sum over m along first dimension and leave row vector indexed in k

Lavg = L + epsilon^2 * (D   - w*temp );         % size [1,K]
Savg = S + epsilon^2 * (D-w - w*temp )./(2*S);  % size [1,K]
 

%--------------------------------------------------------------------------
% sum(...,1) sum over m along first dimension and leave row vector indexed in k
Urmse = epsilon     * sqrt(             w/T * sum(           (L(m)+L(k))./(L(m)-L(k)).^2 , 1) );    % size [1,K]
Vrmse = epsilon./S .* sqrt( (D-w)/D  +  w/D * sum( L(m) .* (3*L(k)-L(m))./(L(m)-L(k)).^2 , 1) );    % size [1,K]

% Cap rmse to limit of pure noise
Urmse = min( Urmse, sqrt(2/T) ); % size [1,K]
Vrmse = min( Vrmse, sqrt(2/D) ); % size [1,K]

% sine of the canonical angles
Usin = sqrt(T)*Urmse .* sqrt( 1 - (T/4)*Urmse.^2 ); % size [1,K]
Vsin = sqrt(D)*Vrmse .* sqrt( 1 - (D/4)*Vrmse.^2 ); % size [1,K]

% cosine of the canonical angles
Ucos = 1 - (T/2)*Urmse.^2; % size [1,K]
Vcos = 1 - (D/2)*Vrmse.^2; % size [1,K]

%--------------------------------------------------------------------------
% expected value (average over many Monte-Carlo trials) of the mode shapes
Uavg = U .* repmat( Ucos, T,1);  % size [T,K]
Vavg = V .* repmat( Vcos, D,1);  % size [D,K]


%--------------------------------------------------------------------------
end % function SVD_Perturbation_Theory_v6
%--------------------------------------------------------------------------




%--------------------------------------------------------------------------
% SVD_Perturbation_Theory_RMSE_v6.m
% Dr. Brenden Epps
% 23-Apr-2019
%
% This function computes the expected 'root mean square error' of the  
% noisy left and right singular vectors (UM and VM) using the  
% perturbation theory detailed in (Epps and Krivitzky 2019a,b).
%
% Inputs:
%   S       = singular values of clean data matrix A = U*S*V'
%             -- S may be a matrix or an array of singular values
%             -- If S unknown, can use noisy SM from noisy M = UM*SM*VM' 
%
%   epsilon = rms error between noisy data matrix M and the clean data A
%
%  [T,D]    = size(A),  where T < D
%
%   w       = effective smoothing window width (w=1 for i.i.d. noise) (see references).
%
% Outputs:
%   Urmse,Vrmse     root mean square error between UM and U, or VM and V.
%                   size [1,K], where K = T = number of modes.
%--------------------------------------------------------------------------
% Version notes:
% v2  calculations vectorized but no changes to the theory.
%
% v3  introduced theory for spatially-correlated data, via "effective smoothing window width", 'w'.
%     used w for modes 1:KF-1 then w=1 for modes KF:K,  where KF = find( S < epsilon*sqrt(T*D), 1,'first')
%
% v4  uses w for all modes.  final version used in (Epps and Krivitzky 2019a,b) papers.
%
% v5  same algorithm as v4, but with code cleanup. 
%     [Usin,Vsin] removed.
%
% v6  release version.  same algorithm as v5, but with code cleanup.
%--------------------------------------------------------------------------
% Reference:
% 1. Brenden Epps and Eric Krivitzky (2019a) "Singular value decomposition of noisy data: mode corruption", Experiments in Fluids 60:121, https://doi.org/10.1007/s00348-019-2761-y
% 2. Brenden Epps and Eric Krivitzky (2019b) "Singular value decomposition of noisy data: noise filtering", Experiments in Fluids 60:126, https://doi.org/10.1007/s00348-019-2768-4
%--------------------------------------------------------------------------

function [Urmse,Vrmse] = SVD_Perturbation_Theory_RMSE_v6(S,epsilon,T,D,w)

if nargin < 5
    w = 1;  % value for i.i.d. noise
end

[N1,N2] = size(S);

if N1 > 1 && N2 > 1 % S is a matrix
    S = diag(S);  % S is now a vector
end

K = length(S);  % K = number of modes

if T > D
    error('SVD_Perturbation_Theory.m is only coded for T < D case!')
elseif K ~= T
    error('number of singular values inconsistent with given size T.')
end

S = S(:)';  % S is now a row vector of size [1,K]
L = S.^2;   

%--------------------------------------------------------------------------
% In order to speed up the code, vectorize all calculations, and 
% perform summations over one matrix index. Define m and k as matrices.  
% Perform sum( ..., 1) to sum over m down each column and leave a row array  
% indexed by k.
m = zeros(K-1,K);

for k = 1:K
    m(:,k) = setdiff( [1:K]', k );  % m changes down each column
end

k = repmat([1:K], K-1,1);  % k is constant down each column and changes across each row
                                             
% sum(...,1) sum over m along first dimension and leave row vector indexed in k
Urmse = epsilon     * sqrt(             w/T * sum(           (L(m)+L(k))./(L(m)-L(k)).^2 , 1) );    % size [1,K]
Vrmse = epsilon./S .* sqrt( (D-w)/D  +  w/D * sum( L(m) .* (3*L(k)-L(m))./(L(m)-L(k)).^2 , 1) );    % size [1,K]

% Cap rmse to limit of pure noise
Urmse = min(Urmse, sqrt(2/T) );
Vrmse = min(Vrmse, sqrt(2/D) );

%--------------------------------------------------------------------------
end % function
%--------------------------------------------------------------------------



%--------------------------------------------------------------------------
% SVD_Perturbation_Theory_UVstd_v6.m
% Brenden Epps
% 23-September-2019
%
% This function computes the standard deviation of the 
% noisy left and right singular vectors (UM and VM) using the  
% perturbation theory detailed in (Epps and Krivitzky 2019a,b).
%
% Inputs:
%   U,S,V   = SVD of clean data matrix A = U*S*V'
%             -- S may be a matrix or an array of singular values
%             -- If {U,S,V} unknown, can use noisy {UM,SM,VM} from noisy data M = UM*SM*VM'
%
%   epsilon = rms error between noisy data matrix M and the clean data A
%
%   w       = effective smoothing window width (w=1 for i.i.d. noise) (see references).
%
% Outputs:
%   Ustd, Vstd  = standard deviation of the noisy left and right singular vectors (UM and VM)
%--------------------------------------------------------------------------
% Version notes:
%   SVD_Perturbation_Theory_UVstd_v6.m computes Ustd and Vstd using code 
%   cut-pasted from SVD_Perturbation_Theory_v4.m, which was the final code
%   used for papers (Epps and Krivitzky 2019a,b).
%--------------------------------------------------------------------------
% Reference:
% 1. Brenden Epps and Eric Krivitzky (2019a) "Singular value decomposition of noisy data: mode corruption", Experiments in Fluids 60:121, https://doi.org/10.1007/s00348-019-2761-y
% 2. Brenden Epps and Eric Krivitzky (2019b) "Singular value decomposition of noisy data: noise filtering", Experiments in Fluids 60:126, https://doi.org/10.1007/s00348-019-2768-4
%--------------------------------------------------------------------------


function [Ustd,Vstd] = SVD_Perturbation_Theory_UVstd_v6(U,S,V,epsilon,w)

if nargin < 5
    w = 1;  % value for i.i.d. noise
end

[N1,N2] = size(S);

if N1 > 1 && N2 > 1 % S is a matrix
    S = diag(S);    % S is now a vector
end

T = size(U,1);
D = size(V,1);
K = length(S);

if T > D
    error('SVD_Perturbation_Theory.m is only coded for T < D case!')
elseif K ~= T
    error('number of singular values inconsistent with T = size(U,1).')
end

L = ( S(:)' ).^2;  % L is  a row vector of size [1,K] 

%--------------------------------------------------------------------------        
Ustd  = zeros(size(U));  % size(U) == [T,K]
Vstd  = zeros(size(V));  % size(V) == [D,K]

for k = 1:K   % for each mode
    m = setdiff( [1:K]', k );

    Ustd(:,k) = epsilon * sqrt(                        w * sum( repmat(                (L(m)+L(k))./(L(m)-L(k)).^2, T,1) .* U(:,m).^2, 2) );  % sum(...,2) sum over m along second dimension
    
    Vstd(:,k) = epsilon * sqrt( (1-w*V(:,k).^2)/L(k) + w * sum( repmat( L(m)/L(k) .* (3*L(k)-L(m))./(L(m)-L(k)).^2, D,1) .* V(:,m).^2, 2) );    
end

%--------------------------------------------------------------------------        
end % function SVD_Perturbation_Theory_UVstd_v6
%--------------------------------------------------------------------------        




%--------------------------------------------------------------------------
% SVD_Estimate_Error_v7.m
% Dr. Brenden Epps
% 24-Sep-2019
%
% This function uses a set of noisy singular values, SM, in order to estimate the
% rms error, epsilon, between a noisy data set M and its clean counterpart A.
% 
% epsilon = sqrt( sum( (M(:)-A(:)).^2 )/(T*D) )
%         = rms error between the noisy data M and the clean data A
%
% This epsilon is estimated by fitting a distribution of singular values SE1
% to the tail of the SM distribution.  The shape of SE1 depends on the size
% [T,D] of matrix M, and whether the noise is spatially correlated (i.e. 
% correlated across rows).  Data with spatially-correlated noise can be
% modeled using a 'spatial correlation factor' f, which alters the shape
% of SE1. The fitted singular values SE1 are proportional to the error  
% level, epsilon1, of the fit.  This epsilon1 is used to predict epsilon.
% 
%
% Inputs:
%   SM      singular values of noisy data matrix M.  size(SM) = [T,1]
%   [T,D]   size of noisy data matrix M.  ***This code assumes T <= D.***
%   f_in    prescribed value of f  (f=1 for i.i.d. data, and f>1 for spatially-correlated data)
%
% Outputs:
%  epsilon   estimate of rms error contained in M
%  epsilon1  error level used to determine SE1 for best fit to the tail of SM.
%  SE1       distribution of singular values fit to tail of SM.
%            SE1 = epsilon1 * SEunit, where SEunit is a function of (T,D,f).
%  f         'spatial correlation factor' used to compute SEunit.
%
%-------------------------------------------------------------------------- 
% Version notes:
% v1  created as SVD_tail_match.m by Eric Krivitzky on December 9, 2017.
%     Edited by Brenden Epps on March 15, 2018.
%
% v6  final version used for Epps and Krivitzky (2019a,b) papers.
%
% v7  same algorithm as *_v6 version, but with code cleaned up and 
%     notation made consistent with papers.
%
%--------------------------------------------------------------------------
% Reference:
% 1. Brenden Epps and Eric Krivitzky (2019a) "Singular value decomposition of noisy data: mode corruption", Experiments in Fluids 60:121, https://doi.org/10.1007/s00348-019-2761-y
% 2. Brenden Epps and Eric Krivitzky (2019b) "Singular value decomposition of noisy data: noise filtering", Experiments in Fluids 60:126, https://doi.org/10.1007/s00348-019-2768-4
%--------------------------------------------------------------------------

function [epsilon, epsilon1, SE1, f] = SVD_Estimate_Error_v7(SM,T,D,f_in)

% set default values for optional inputs
if nargin < 4
    f_in = NaN;  
end

% check inputs
if T > D
    error('Sorry, T > D is not curretly supported.')
else
    K = T;  % number of modes
    Kmin = max(floor( 0.10 * K ), 1);  % minimum index to consider as k_epsilon
    Kmax = max(floor( 0.80 * K ), 2);  % maximum index to consider as k_epsilon
    Kerr = K;                          % maximum index to consider for computing error between SM and fit.
end

% ensure SM is the correct dimension:
SM = SM(:)';  % [1,K]

%--------------------------------------------------------------------------
% Prescribe a set of f to try:
if isnan(f_in)
  f_all = [1:0.1:min(12,floor(D/T))]';   
else
  f_all = f_in;
end

Nd = length(f_all);  % number of f to try


%--------------------------------------------------------------------------
% Compute singular values of unit-noise matrix:
% Marchenko-Pastur distribution with epsilon=1 and 'spatial correlation factor' f
SEunit = zeros(Nd,K);

for d = 1:Nd
    SEunit(d,:) = MarchenkoPastur_v4(T,D, 1, f_all(d));  
end

log10_SEunit = log10(SEunit);  
log10_SM     = repmat(log10(SM), Nd,1);   % now size [Nd,K]


%--------------------------------------------------------------------------
% Determine best-fit error levels, epsilon1, and associated fit error.
log10_epsilon1 = NaN(Nd,Kmax);
fit_error      = NaN(Nd,Kmax);
  
% fit the Marchenko-Pastur distribution to the noisy singular values
for k = Kmin:Kmax  % for each index to check

    log10_epsilon1(:,k) = mean(  log10_SM(:,k:Kerr) - log10_SEunit(:,k:Kerr),  2);  % mean(...,2) takes mean along the second dimension

    fit_error(:,k)      = mean( (log10_SM(:,k:Kerr) - log10_SEunit(:,k:Kerr) - repmat(log10_epsilon1(:,k),1,length(k:Kerr)) ).^2 , 2);
end

  
%--------------------------------------------------------------------------
% Find minimum error and index location   
[~, minloc_eps] = min( log10_epsilon1, [],2 );   % min(...,[],2) takes the minimum along the 2nd dimension

minloc        = zeros(Nd,1);  % k index location of minimum fit error for each d 
min_fit_error = zeros(Nd,1);  % value of minimum fit error (minimum along k index direction) (minimum error for each d)
epsilon1      = zeros(Nd,1);  % error level for noisy data tail fit
epsilon2      = zeros(Nd,1);  % error level estimate for actual error data

for d = 1:Nd
    
    [min_fit_error(d), minloc(d)] = min( fit_error(d,1:minloc_eps(d)) );  

    
    epsilon1(d) =    10^log10_epsilon1(d,minloc(d));  % error level for M-P distribution best fit to tail of SM   (empirically found to be larger than true epsilon)
    
    % estimate true error:
    epsilon2(d) = epsilon1(d) * SEunit(d,minloc(d)) / sqrt(D);   % empirically found to be less than true epsilon
end

%--------------------------------------------------------------------------
% Choose the best fit among all f... as the one that gives the overall minimum fit error
[fit_error,d] = min( min_fit_error );  % minimum of all min_fit_error
   
f          = f_all(d);
minloc     = minloc(d);
epsilon1   = epsilon1(d);
epsilon2   = epsilon2(d);
SEunit     = SEunit(d,:);

SE1  = epsilon1 * SEunit';  % [K,1] distribution of 'error' singular values fit to tail of S.

%--------------------------------------------------------------------------
% Best estimate of true epsilon:

KE1 = find(SM < epsilon1*sqrt(D), 1);  % key index location

% Set epsilon = epsilon2  if  minloc==KE1, and 
% set epsilon = epsilon1  if  minloc==Kmax. min(max( x ,0),1) ensures 0 <= x <= 1
epsilon = epsilon2 + (epsilon1-epsilon2) *  min(max( (minloc-KE1)/(Kmax-KE1) ,0),1);


%--------------------------------------------------------------------------
end % function SVD_Estimate_Error_v7
%--------------------------------------------------------------------------


%--------------------------------------------------------------------------
% MarchenkoPastur_v4.m
% Dr. Brenden Epps
% 23-Sep-2019
%
% This function returns the expected distribution of singular values S of a
% matrix X of random data with the following properties:
%   -- size(X) == [T,D] where f*T < D,
%   -- normal (Gaussian) distribution with zero mean and standard deviation epsilon,
%   -- the data may be independent identically-distributed (i.i.d.) or they may be
%      spatially-correlated (i.e. correlated across rows).   
%
% For example, such a dataset X -- with normal distribution, zero mean, 
% standard deviation  epsilon, and uniform spatial smoothing (across rows)   
% with window width w -- can be obtained via uniform spatial smoothing as follows:
%
%     X = sqrt(w)*epsilon*randn(T,D);	% normal dist., standard deviation = sqrt(w)*epsilon
% 
%     for t = 1:T
%         X(t,:) = smoothdata(X(t,:),'movmean', w); % uniform spatial smoothing... now X is normal with standard deviation = epsilon
%     end
%   
% This 'smoothing window width' w is monotonically related to a 'spatial
% coorelation factor' f described below.  See references.
%
%
% The distribution of singular values S is computed via the Marchenko-Pastur
% Law with possible modification for spatially-correlated data. The
% 'spatial correlation factor' f is introduced here to account for the
% possibility of spatial correlation of the data (i.e. smoothing of X 
% across rows).  The original Marchenko-Pastur distribution has f = 1 and 
% corresponds to i.i.d. random data. Setting f > 1 yields a distribution of 
% singular values that well represents those obtained by smoothing of X 
% across rows, which introduces spatial correlation (if each row is 
% conceptualized as a snapshot in time, and each column is concptualized as 
% the data from one spatial location). We have empirically found this 
% approximation works very well when D/T > 20 and D/(f*T) > 5. See
% (Epps and Krivitzky 2019a) Appendix 5.
%
%
% Inputs: 
%   T,D         size of the random data matrix, X
%   epsilon     standard deviation of population from which the sample X was drawn
%   f           spatial correlation factor (f=1 for i.i.d. data, and f>1 for spatially-correlated data)
%
% Outputs:
%   S           expected singular values of X (vector of length T)
%
%-------------------------------------------------------------------------- 
% Version notes:
% v1  original implementation without f
%
% v3  introduced the 'spatial correlation factor' f
%     v3 was the final version used in (Epps and Krivitzky 2019a,b)
%
% v4  release version.  same algorithm as v3, but with code cleanup.
%--------------------------------------------------------------------------
% Reference:
% 1. Brenden Epps and Eric Krivitzky (2019a) "Singular value decomposition of noisy data: mode corruption", Experiments in Fluids 60:121, https://doi.org/10.1007/s00348-019-2761-y
% 2. Brenden Epps and Eric Krivitzky (2019b) "Singular value decomposition of noisy data: noise filtering", Experiments in Fluids 60:126, https://doi.org/10.1007/s00348-019-2768-4
%--------------------------------------------------------------------------

function S = MarchenkoPastur_v4(T,D,epsilon,f)

if nargin == 3 
    f = 1;  % default value 
end  

if D < f*T  
    error('D must be greater than f*T')
end  

Plot_flag = 0;

%--------------------------------------------------------------------------
% Implementation notes:
%
% 1. Without loss of generality, assume size(X) = [T,D] with T < D.
%
%    The elements of X are drawn from a population with normal (Gaussian) 
%    distribution with zero mean and a standard deviation of epsilon.
%
% 2. The Marchenko-Pastur Law relates to the distribution of eigenvalues 
%    of matrix Y = (X*X')/(D*epsilon^2).  
%
%      (singular values of X) = sqrt( eigenvalues of X*X' )
%                             = sqrt( (D*epsilon^2) * (eigenvalues of Y) )
%
% 3. size(X*X') = [T,T], so we seek T singular values (T eigenvalues).
%
% 4. The Marchenko-Pastur Law gives the probability density function, f(z),
%    where f(z)*dz is the probability of eigenvalues lambda in the range 
%    z - dz/2 <= lambda <= z + dz/2. 
%     
%         f(z) = 1/(2*pi*y) * sqrt((b-z).*(z-a))./z      (Marchenko-Pastur)
%
%    where
%           a  = (1-sqrt(y))^2;      % expected value of the smallest eigenvalue
%           b  = (1+sqrt(y))^2;      % expected value of the  largest eigenvalue
%           y  = T/D;                % y < 1.   note: T/y = D
%    
% 5. Let  F(z) be the cumulative distribution function, which is the fraction 
%    of eigenvalues lambda in the range a <= lambda <= z.  By definition, the 
%    cumulative distribution function is the integral of the probability 
%    density function:
%
%                z
%    F(z) = integral  f(z') * dz'
%                a
%
% 6. F(z) can be evaluated analytically, and the analytic result is used herein.
%
% 7. The output singular values are evenly distributed in F(z).  That is,
%    we find the eigenvalues z_out corresponding to even spacing
%    F_out = 1:-1/(T-1):0, such that z_out(1) = b and z_out(T) = a.
%
% 8. The singular values of X are:  S = sqrt( D * epsilon^2 * z_out )
%--------------------------------------------------------------------------

%    
%--------------------------------------------------------------------------
% Analytic Implementation:  z = eigenvalue of matrix (X*X')/(D*epsilon^2)
%
Nz = max(1000,4*T);      % number of z values to include in the integral
y  = f*T/D;              % Requirement:  y < 1

a = ( 1 - sqrt(y) )^2;  %  smallest eigenvalue,  a == (sqrt(D) - sqrt(f*T))^2/D
b = ( 1 + sqrt(y) )^2;  %   largest eigenvalue,  b == (sqrt(D) + sqrt(f*T))^2/D

z  = linspace(a,b,Nz);   %   dummy variable of integration,  continuum of eigenvalues

% Fz = F(z) fraction of eigenvalues in range [a,z]
Fz = 1/(2*pi*y) * ( ...
                    sqrt(a*b)*( 2*atan(sqrt((a*(b-z))./(b*(z-a)))) - pi ) ...
                  + 0.5*(a+b)*(   atan((z-0.5*(a+b))./sqrt((b-z).*(z-a))) + pi/2 )  ...
                  + sqrt((b-z).*(z-a))  );

              
% % Check that F(z=a) == 0 and F(z=b) == 1:
% [Fz(1), Fz(Nz)] % == [0, 1]


% Interpolate to find eigenvalues corresponding to a linear spacing in probability
F_out = 1:-1/(T-1):0;         % desired F(z) values: linear spacing in probability
z_out = pchip(Fz, z, F_out);  % eigenvalues z_out correspondig to F(z=z_out) == F_out
S     = sqrt( D * epsilon^2 * z_out );   % S = singular value of X corresponding to eigenvalue z_out
%--------------------------------------------------------------------------


%--------------------------------------------------------------------------
if Plot_flag == 1
    % Sz = singular value of X corresponding to eigenvalue z 
      Sz = sqrt( D * epsilon^2 * z );   
    
    % Need to make up an index such that Fz = 1 corresponds to Sz_index = 1 and
    %                                    Fz = 0 corresponds to Sz_index = T
     Sz_index = T - Fz*(T-1);
  % [Sz_index(1),Sz_index(end)]
    
        
    figure(1), clf,  hold on, box on, grid on,  
        set(gca,'YScale','log');

        plot(Sz_index,Sz,'k')

        plot([1:T],S,'or')  % output singular values, plotted versus index


    figure(2), clf,  hold on, box on, grid on,
        plot(Sz,Fz,'k')     
        plot(S ,F_out,'or')      
end
%--------------------------------------------------------------------------

%--------------------------------------------------------------------------
end % function MarchenkoPastur_v4
%--------------------------------------------------------------------------



%--------------------------------------------------------------------------
% SVD_ComputeGap_v2.m
% Dr. Brenden Epps
% 23-Sep-2019
%
% This function computes the "gap" between singular values, where
%
%   gap = minimum difference between neighboring singular values.
%
%
% Inputs:
%   S  [K,1] or [1,K] vector of singular values (given in descending order)
%
% Output:
%   gap   [same size as S]
%
%-------------------------------------------------------------------------- 
% Version notes:
% v1  implementation:
%
%         K = length(S);
% 
%         Sgap = zeros(size(S));
% 
%              Sgap(1) =                       S(1) - S(2);
%         for k = 2:K-1
%              Sgap(k) = min(  S(k-1) - S(k),  S(k) - S(k+1) );
%         end
%              Sgap(K) = min(  S(K-1) - S(K),  S(K)          );  
%
%
% v2  implementation vectorizes the above calculations.
%--------------------------------------------------------------------------
% Reference:
% 1. Brenden Epps and Eric Krivitzky (2019a) "Singular value decomposition of noisy data: mode corruption", Experiments in Fluids 60:121, https://doi.org/10.1007/s00348-019-2761-y
% 2. Brenden Epps and Eric Krivitzky (2019b) "Singular value decomposition of noisy data: noise filtering", Experiments in Fluids 60:126, https://doi.org/10.1007/s00348-019-2768-4
%--------------------------------------------------------------------------

function Sgap = SVD_ComputeGap_v2(S)

[R,C] = size(S);    % R = number of rows, C = number of columns

if R > 1 && C == 1  % column vector
    
    K = R;
    
    Sgap = [S(1) - S(2);  min(  S(1:K-2) - S(2:K-1),  S(2:K-1) - S(3:K) ); ...
                          min(  S(K-1)   - S(K)    ,  S(K)              )];

elseif R == 1 && C > 1  % row vector
    
    K = C;
    
    Sgap = [S(1) - S(2),  min(  S(1:K-2) - S(2:K-1),  S(2:K-1) - S(3:K) ), ...
                          min(  S(K-1)   - S(K)    ,  S(K)              )];
                      
else
    error('S must be a [K,1] or [1,K] vector')
end

%--------------------------------------------------------------------------
end % function SVD_ComputeGap_v2
%--------------------------------------------------------------------------

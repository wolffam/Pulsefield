% Run the entire calibration/analysis chain
doplot=2;
dolevcheck=false;

if ~exist('p')
  disp('Initializing setup')
  p=struct;
  p.analysisparams=analysissetup();
  ctype='av10115';
  %  ctype='av10115-half';
  ids=[1 2 5 3 4];   % In order around circle
  ncamera=length(ids);
  for i=1:ncamera
    p.camera(i)=setupcamera(ctype,ids(i));
  end
  p.led=struct('id',num2cell(1:numled()));
  p.colors={[1 1 1], [1 0 0], [0 1 0], [0 0 1],[1 1 0],[1 0 1], [0 1 1]};

  p.layout=layoutpolygon(6,ncamera,0);
  plotlayout(p.layout);
end

% Set camera exposures
fprintf('Setting camera exposures\n');
for id=[p.camera.id]
  arecont_set(id,'autoexp','on');
  arecont_set(id,'exposure','on');
  arecont_set(id,'brightness',0);
  arecont_set(id,'lowlight','highspeed');
  if strcmp(p.camera(1).type,'av10115')
    arecont_set(id,'shortexposures',2);
  else
    arecont_set(id,'shortexposures',2);
  end
  arecont_set(id,'maxdigitalgain',32);
  arecont_set(id,'analoggain',1);
end

if ~isfield(p.camera(1),'pixcalib')
  disp('Pixel calibration');
  if exist('pixcalibimages')
    p=pixcalibrate(p,pixcalibimages);
  else
    [p,pixcalibimages]=pixcalibrate(p);
  end
  if doplot
    for i=1:length(p.camera)
      pixcalibplot(p.camera(i));
    end
    plotdistortion(p);
  end
end

if ~isfield(p.camera(1),'viscache')
  disp('Visible calibration');
  [allvis,p]=getvisible(p,'init');
end

if ~isfield(p,'crosstalk')
  % Only really needed 
  disp('Crosstalk calculation');
  p=crosstalk(p);
end

% Refine position of cameras
% Need to physically block cameras for this...
% Instead use prior calibration
cposcalib=[ 1.0361   -2.2656
            -1.3310   -2.1188
            -1.3218    2.1082
            1.0443    2.2606];
if length(p.layout.cpos)==4 && max(max(abs(cposcalib-layout.cpos)))<0.1
  fprintf('Forcing camera positions to previously calibrated values\n')
  p.layout.cpos=cposcalib;
else
  %layout.cpos=locatecameras();
  fprintf('*** Should run locatecameras()\n');
end

% Use the layout and the pixcalibration to update the anglemaps and the camera cdir
p=updateanglemap(p);

% Could also adjust positions of LEDs based on pixel calibration, but this doesn't work yet
%adjpos(p)

% Add ray image to structure (rays from each camera to each LED) to speed up target blocking calculation (uses true coords)
if ~isfield(p,'rays')
  disp('Precomputing rays');
  p.rays=createrays(p);
end

disp('Measuring');
vis=getvisible(p,'stats',true);
if doplot>1
  plotvisible(p,vis);
end

% Analyze data to estimate position of targets using layout
samp=analyze(p,vis.v,doplot);

% Run recording
%recvis=recordvis(p,5);
%recanalyze(recvis); % Analyze whole recording to get tracking
%recanalyze(recvis,2);   % Analyze specific sample

if dolevcheck
  disp('Check levels');
  levcheck(p,1);
end

% Model x,y locations as a pair leg and return struct holding:
%   c1 - center of leg1
%   c2 - center of leg2
%   radius - radius
%   err - mean square error
%   assignment - array of 1 or 2 specifying leg to point assignments
% Assume scan origin is 0,0
function legs=legmodel(vis,class,varargin)
  defaults=struct('maxlegdiam',0.3,...   % Maximum leg diameter
                  'minlegdiam',0.1,...   % Minimum
                  'maxlegsep',0.3,...
                  'search',false,...
                  'debug',false...
                  );
  args=processargs(defaults,varargin);

  r=(args.minlegdiam+args.maxlegdiam)/4;

  if sum(vis.class==class)==0
    legs=struct('c1',[nan,nan],'c2',[nan,nan],'radius',r,'err',0);
    return;
  end
  xy=vis.xy(vis.class==class,:);
  shadowed=vis.shadowed(vis.class==class,:);
  leg=vis.leg(vis.class==class,:);
  range=vis.range(vis.class==class);
  angle=vis.angle(vis.class==class);
  npts=size(xy,1);
  res=vis.angle(2)-vis.angle(1);

  % Fast heuristics
  if all(leg==0)
    % Just model as one big leg
    theta1=max(angle)-min(angle)+res;
    range1=mean(range);
    r=range1*theta1/2;
    if args.debug
      fprintf('1-leg: range=%.2f, r=%.3f\n', range1,r);
    end
    % Use single diameter, since this could be a single leg (other one behind)
    if r*2<args.minlegdiam
      if args.debug
        fprintf('Diameter too small (%f), setting to %f\n', r*2, args.minlegdiam);
      end
      r=args.minlegdiam;
    end
    % Use double diameter, since this could be fusion of 2 legs
    if r*2>args.maxlegdiam*2
      if args.debug
        fprintf('Diameter too large (%f), setting to %f\n', r*2, args.maxlegdiam*2);
      end
      r=args.maxlegdiam;
    end
  else
    theta1=max(angle(leg==1))-min(angle(leg==1))+res;
    range1=mean(range(leg==1));
    theta2=max(angle(leg==2))-min(angle(leg==2))+res;
    range2=mean(range(leg==2));
    r=max([range1*theta1,range2*theta2])/2;
    if args.debug
      fprintf('Initial: range1=%.2f, r1=%.2f, range2=%.2f, r2=%.3f r=%.3f\n', range1,range1*theta1/2, range2,range2*theta2/2, r);
    end
    if r*2<args.minlegdiam
      if args.debug
        fprintf('Diameter too small (%f), setting to %f\n', r*2, args.minlegdiam);
      end
      r=args.minlegdiam/2;
    end
    if r*2>args.maxlegdiam
      if args.debug
        fprintf('Diameter too large (%f), setting to %f\n', r*2, args.maxlegdiam);
      end
      r=args.maxlegdiam/2;
    end
  end

  % Set position of first leg (or body if there is only one leg)
  anglewidth1=2*r/range1;
  shadowed1=shadowed(leg<=1,:);
  if shadowed1(1,1)&~shadowed1(end,2)
    angle1=angle(find(leg<=1,1,'last'))+res/2-anglewidth1/2;
  elseif ~shadowed1(1,1)&shadowed1(end,2)
    angle1=angle(find(leg<=1,1))-res/2+anglewidth1/2;
  else
    angle1=mean(angle(leg<=1));
  end
  [c1(1),c1(2)]=pol2cart(angle1+pi/2,range1+r);

  if ~all(leg==0)
    % Set position of 2nd leg
    anglewidth2=2*r/range2;
    shadowed2=shadowed(leg==2,:);
    if shadowed2(1,1)&~shadowed2(end,2)
      angle2=angle(find(leg==2,1,'last'))+res/2-anglewidth2/2;
      range2=range2-r/4;   % Reduced distance since we dont have the near points
    elseif ~shadowed2(1,1)&shadowed2(end,2)
      angle2=angle(find(leg==2,1))-res/2+anglewidth2/2;
      range2=range2-r/4;   % Reduced distance since we dont have the near points
    else
      % Assume middle if both or neither edges are shadowed
      angle2=mean(angle(leg==2));
    end
    [c2(1),c2(2)]=pol2cart(angle2+pi/2,range2+r);
  else
    c2=[nan,nan];
  end
  
  
if args.debug
  plotresults(class,r,c1,c2,args,xy,leg);
end
  for i=1:2
    xy(leg<=1,i)=xy(leg<=1,i)-c1(i);
    xy(leg==2,i)=xy(leg==2,i)-c2(i);
  end
  err=mean((sqrt(xy(:,1).^2+xy(:,2).^2)-r).^2);
  ass=leg;
  legs=struct('c1',c1,'c2',c2,'radius',r,'err',sqrt(err),'assignment',ass);
end

function plotresults(class,r,c1,c2,args,xy,leg)
  setfig(sprintf('legmodel%d',class));clf;
  plot(xy(leg==0,1),xy(leg==0,2),'x');
  hold on;
  plot(xy(leg==1,1),xy(leg==1,2),'>');
  plot(xy(leg==2,1),xy(leg==2,2),'<');
  plot(c1(1),c1(2),'o');
  plot(c2(1),c2(2),'o');
  angle=-pi:.01:pi;
  [x,y]=pol2cart(angle,r);
  x1=x+c1(1);
  y1=y+c1(2);
  plot(x1,y1);
  x2=x+c2(1);
  y2=y+c2(2);
  plot(x2,y2);
  axis equal
end     
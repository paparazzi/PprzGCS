#include "gvf_traj_bezier.h"
#include <stdio.h>
#include <stdlib.h>

GVF_traj_bezier::GVF_traj_bezier(QString id, QList<float> param, QList<float> _phi, 
				  float wb, QVector<int> *gvf_settings) :
    GVF_trajectory(id, gvf_settings)
{   
    set_param(param, _phi, wb);
    generate_trajectory();
}

// 2D bezier trajectory (parametric representation)
void GVF_traj_bezier::genTraj() { 
    QList<QPointF> points;
    
    float res = 1e7;
    float N;


    // TODO: Change num_pts acrroding ttoo things
    float max_t = n_seg; // NUMBER OF SPLINES max_t = NUMBER OF SPLINES.. TODO
    float num_pts = max_t*100; // 100 per each segment

    float dt = max_t /num_pts;
    for (float t = 0; t <= max_t + dt/2; t+=dt) {
        points.append(traj_point(t));
    }
    createTrajItem(points);
}


// 2D bezier GVF 
void GVF_traj_bezier::genVField() { 


    QList<QPointF> vxy_mesh; 

    float xmin, xmax, ymin, ymax;
    xmin = xx[0]; xmax = xx[0]; ymin = yy[0]; ymax = yy[0];
    for(int k = 0; k < 3*n_seg + 1; k++)
    {
    	if(xx[k] > xmax)
    		xmax = xx[k];
    	if(xx[k] < xmin)
    		xmin = xx[k];
    	if(yy[k] > ymax)
    		ymax = yy[k];
    	if(yy[k] < ymin)
    		ymin = yy[k];
    }  
    //float bound_area = 2*(xmax-xmin)*(ymax-ymin);
    float bound_area = 2*(xmax-xmin)*(ymax-ymin);
    
    // 40x40 is the number of arrows in x and in y
    emit DispatcherUi::get()->gvf_defaultFieldSettings(ac_id, round(bound_area), 40, 40);
    xy_mesh = meshGrid();
    
    foreach (const QPointF &point, xy_mesh) {
	
	float phix = point.x() - traj_point(w).x(); // Normal component
	float phiy = point.y() - traj_point(w).y(); // Normal Component
	
	float sigx = beta*traj_point_deriv(w).x(); // Tangential Component
	float sigy = beta*traj_point_deriv(w).y(); // Tangential Component
	
	float vx = sigx - kx*phix;
	float vy = sigy - ky*phiy;

        float norm = sqrt(pow(vx,2) + pow(vy,2));

        vxy_mesh.append(QPointF(vx/norm, vy/norm));
    }

    createVFieldItem(xy_mesh, vxy_mesh);
}

/////////////// PRIVATE FUNCTIONS ///////////////
void GVF_traj_bezier::set_param(QList<float> param, QList<float> _phi, float wb) {
    if (param.size()>15) { // gvf_parametric_2D_bezier_wp()
        auto ac = pprzApp()->toolbox()->aircraftManager()->getAircraft(ac_id);
        Waypoint::WpFrame frame = ac->getFlightPlan()->getFrame();
    }


    // Lest test writing on a file the parameters, and then take the information from that file... (.txt file)
    // TODO: Remove files when the program ends...
    // TODO: join in one file. x in first line, y in second line, ks in third line. getline (or seek \n)? And then the pointer is in the next line¿¿¿¿¿
    
    // Number of P points (P \in R^2) : 3*N_SEGMENTS + 1
    // param[0] = N_SEGMENTS
    FILE *file_x; FILE *file_y; FILE *file_ks; //FILE *file;
    // TODO (ing) = Use only one file
    char x_val[]  = "/tmp/x_values.txt"; // ...
    char y_val[]  = "/tmp/y_values.txt";
    char ks_val[] = "/tmp/ks_values.txt";
    //char file_val[] = "/tmp/bez_values.txt";
    
    int k = 1;
    
    // Write:
    if(param[0] < 0){
    	    n_seg = -(int)param[0];
    	    file_x = fopen(x_val, "w+");
	    for(k = 0; k < 3*n_seg+ 2; k++){
	    	fprintf(file_x, "%f ", param[k]);
	    }
	    fclose(file_x);
    }
    else if(param[0] > 0){
    	    n_seg = (int)param[0];
    	    file_y = fopen(y_val, "w+");
	    for(k = 0; k < 3*n_seg + 2; k++){
	    	fprintf(file_y, "%f ", param[k]);
	    }
	    fclose(file_y);
    }
    else{
    	file_ks = fopen(ks_val, "w+");
    	fprintf(file_ks, "%f %f %f", param[1], param[2], param[3]);
    	fclose(file_ks);
    }
    
   float N_SEG;
   // Read:
   if( (file_x = fopen(x_val, "r")) != NULL)
   {
   	    fscanf(file_x, "%f ", &N_SEG);
   	    n_seg = -(int)N_SEG;
	    for(k = 0; k < 3*(int)n_seg + 1; k++){
	    	fscanf(file_x, "%f ", &xx[k]);
	    }
	    fclose(file_x);
   }
   if( (file_y = fopen(y_val, "r")) != NULL)
   {
      	    fscanf(file_x, "%f ", &N_SEG);
      	    n_seg = (int)N_SEG;
	    for(k = 0; k < 3*(int)n_seg + 1; k++){
	    	fscanf(file_y, "%f ", &yy[k]);
	    }
	    fclose(file_y);
   }
   if( (file_ks = fopen(ks_val, "r")) != NULL)
   {
   	fscanf(file_ks, "%f", &kx);
   	fscanf(file_ks, "%f", &ky);
   	fscanf(file_ks, "%f", &beta);
   	fclose(file_ks);
   }
   
   /*
   // Read
    if(param[0] < 0){
    	    n_seg = -(int)param[0];
    	    file = fopen(file_val, "wr+");
	    for(k = 0; k < 3*n_seg+ 2; k++){
	    	fprintf(file_val, "%f ", param[k]);
	    }
	    fprintf(file_val, "%c", '\n');
	    fclose(file_val);
    }
    else if(param[0] > 0){
    	    n_seg = (int)param[0];
    	    file_y = fopen(file_val, "wr+");
    	    // One must write in the next line..
    	    // I can check if the file is already created by first reading and if the pointer is not null then it must exist.....
    	    // with that on mind we can then write on the second line. The question is how
	    for(k = 0; k < 3*n_seg + 2; k++){
	    	fprintf(file_val, "%f ", param[k]);
	    }
	    fprintf(file_val, "%c", '\n');
	    fclose(file_val);
    }
    else{
    	file_ks = fopen(file_val, "wr+");
    	fprintf(file_val, "%f %f %f", param[1], param[2], param[3]);
    	fprintf(file_val, "%c", '\n');
    	fclose(file_val);
    }
    */
    phi = QPointF(_phi[0], _phi[1]);   //TODO: Display error in GVF viewer??
    w = wb/beta; 		       // gvf_parametric_w = wb/beta (wb = w*beta)
}

QPointF GVF_traj_bezier::traj_point(float t) {

    int k = (int)t; // It works like a floor .. t >= k
    /* Each Bézier spline must be evaluated between 0 and 1. 
    Remove integer part, get fractional 
    Choose which spline (0<=t<=1 -> Spline1 (coefs 0-3), 
    1<=t<=2 -> Spline2 (coefs 3-6) ... */
    t = t - k;      
    k = 3*k;        
    float x = xx[k]*(1-t)*(1-t)*(1-t) + 3*(1-t)*(1-t)*t*xx[k+1] + 
    	      3*(1-t)*t*t*xx[k+2] + t*t*t*xx[k+3]; 
    float y = yy[k]*(1-t)*(1-t)*(1-t) + 3*(1-t)*(1-t)*t*yy[k+1] + 
     	      3*(1-t)*t*t*yy[k+2] + t*t*t*yy[k+3];   
    return QPointF(x,y);
}

QPointF GVF_traj_bezier::traj_point_deriv(float t) {

    int k = (int)t; // It works like a floor .. t >= k
    /* Each Bézier spline must be evaluated between 0 and 1. 
    Remove integer part, get fractional 
    Choose which spline (0<=t<=1 -> Spline1 (coefs 0-3), 
    1<=t<=2 -> Spline2 (coefs 3-6) ... */
    t = t - k;      
    k = 3*k;        
    float dx = (xx[k+1] - xx[k])*3*(1-t)*(1-t) + 
    		6*(1-t)*t*(xx[k+2]-xx[k+1]) + 3*t*t*(xx[k+3] - xx[k+2]); 
    float dy = (yy[k+1] - yy[k])*3*(1-t)*(1-t) + 
    		6*(1-t)*t*(yy[k+2]-yy[k+1]) + 3*t*t*(yy[k+3] - yy[k+2]);   
    return QPointF(dx,dy);
}

float GVF_traj_bezier::gcd(int a, int b){
    if(b == 0)
        return a;
    return gcd(b, a % b);
}

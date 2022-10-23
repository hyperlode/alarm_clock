
OVERLAP = 3;
translate([-9.5,-25,0]){
cube([19,50,8]);
}
translate([9.5-2.5,25-2.5,-1]){
    cube([2.5,2.5,1+OVERLAP]);
}
translate([-9.5,25-2.5,-1]){
    cube([2.5,2.5,1+OVERLAP]);
}
translate([9.5-2.5,-25,-1]){
    cube([2.5,2.5,1+OVERLAP]);
}
translate([-9.5,-25,-1]){
    cube([2.5,2.5,1+OVERLAP]);
}
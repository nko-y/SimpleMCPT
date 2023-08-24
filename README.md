## 一、运行及说明

（1）本程序所使用的所有外部库均包含于include文件夹下，包括以下几部分：

- glm：用于基本的数学和向量运算
- stb_image/stb_image_write：用于纹理图片的读取以及最终图像的输出
- tiny_obj_loader：用于模型的导入
- tinyxml2：用于xml图像的读取

（2）本项目的运行方式有两种：

- 打开PT.sln项目，并将项目的包含目录添加到其中，在main函数中可以改变spp数量以及对应场景的存放路径

<img src=".\img\vsrun.png" alt="vsrun" style="zoom:40%;" />

- 通过生成的PT.exe直接运行，如下所示打开控制台调用exe并输入图像路径和spp数量即可

<img src=".\img\cmdrun.png" alt="cmdrun" style="zoom:50%;" />

（3）注意事项：

- 场景相关资源(mtl、obj、xml)应当与文件名相同，从而保证程序能够正确读取有关资源

<img src=".\img\sourcename.png" alt="sourcename" style="zoom:50%;" />

- 将材质中的Tr改成Kt，只有这样tiny_obj_loader才能正确地读入玻璃的transmission属性



## 二、整体思路和类实现

（1）首先要做的是场景的导入，其中的核心有四个部分：模型点面法线信息、模型材质信息、光源信息、相机信息。在SceneLoader的构造函数中我们通过路径名解析出资源名称，并依次实现这些信息的读取，对应的伪代码如下所示：

```c++
//读入obj mtl xml文件
tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, scene_path, mtl_path);
xml_loader.LoadFile(xml_path);

//读入光源信息
for (auto node = xml_loader.FirstChildElement("light"); node!=nullptr; node = node->NextSiblingElement("light")){
    std::string light_name = node->Attribute("mtlname");
	std::string light_radiance_string = node->Attribute("radiance");
}

//读入材质信息
for (auto mtl : materials) {
    if(mtl.ior>1.0) this_mtl = new GlassMaterial();
    else this_mtl = new PhongMaterial();
    
    if(light_node.count(mtl.name)>0) this_mtl->SetEmissive();
    if(mtl.diffuse_texname.length()>0) this_mtl->ReadTexture();
}

//读入模型几何信息
for(size_t s=0; s<shapes.size(); s++){
    GetPoints();
    GetNormal();
    GetUV();
}

//读入相机信息
auto camera_node = xml_loader.FirstChildElement("camera");
GetCameraInfo(&eye, &lookat, &up, &fov, &width, &height);
```

（2）在读入以上场景时我们需要构建对应的类存储对应的信息，主要包括：

- 材质：
  - Materia基类，定义了材质的属性(Phong、Glass、Light)以及与蒙特卡洛重要性采样相关的pdf、bsdf、scatter函数
  - PhongMaterial类，为Material的一个子类，代表使用Phong模型计算的材质，如果指定了is_emissive则代表该材质是光源材质
  - GlassMaterial类，为Material的一个子类，代表玻璃等可以投射的材质
  - Texture类，用于存储纹理图片并通过Get(vec2& uv)可以获得纹理图片上一点的材质信息
- 物体：
  - Hittable类，代表了该几何的碰撞属性，定义了hit函数用于求交计算和GetBoundingBox用于获取包围盒
  - BoundingBox类，用于表示该几何体的AABB包围盒属性
  - Emittable类，代表了该几何的发光属性，定义了SampleRay用于光线的采样
  - Triangle类，继承于Hittable类和Emittable类代表三角面片的几何体，新的几何体可以用类似方法定义
  - EmittableCluster类，继承于Emittable代表发光物体的集合体
- 相机：
  - Camera类，记录了读入的所有相机参数，并提前计算图像左上角的世界坐标以及横向移动的世界长度dw与纵向移动的世界长度dh，通过CastRay函数给定像素坐标即可向世界发射对应的光线进行求交运算。

（3）为了将物体有条理地组织起来，我们构造了BVH树结构，他是Hittable类的一个子类。对应的伪代码和构建遍历思路如下所示：

- BuildTree。构建这颗树的基本思路是传入所有空间物体(在这里是三角形)，将所有物体的AABB包围盒合并起来形成一个总的包围盒，选取包围盒最长的轴进行分裂，这里的分裂可以先将所有物体在这个轴上的最小坐标值进行排序。简单的做法是均分，将左半边的物体传入左儿子递归构建树结构，右半边的物体传入右儿子递归构建树结构。也可以从光线射入包围盒命中的概率出发使用SAH BVH的方法，均匀划分n个剖分点，分别计算每个点光线击中包围盒的概率即 $c(A,B)=\frac{S(A)}{S(C)}a+\frac{S(B)}{S(C)}b$ 这里$S(A)S(B)S(C)$分别代表左半边的包围盒表面积、右半边的包围盒表面积、父节点包围盒表面积，而$ab$则代表左右半边图元的数量，我们选择计算出的值最大的那个点作为左右儿子的剖分点，这样可以保证更高的求交效率。

```C++
Hittable* BuildTree(vector<Hittable*>all, int from, int to){
    //遍历结束返回叶子节点
    if(to<=from) return nullptr;
    if(to-from==1) return all[from];
    
    //构建父节点包围盒并选出最长轴
    BoundingBox* C = new Boundingbox(all, from, to);
    int max_axis = SortMaxAxis(C);
    
    //以最长轴进行排序
    std::sort(all.begin()+from, all.begin()+to, comp_fun);
    
    //使用简单的二分或者SAH选择出划分点并构建子节点
    int idx = GetDivideIndex(all, from, to);
    node->child[0] = BuildTree(all, from, idx);
    node->child[1] = BuildTree(all, idx, to);
}
```

- Hit。当检测一条光线与场景相交的时候，我们自上而下遍历整棵树，如果树和左右儿子AABB包围盒都没有交点则说明不相交，如果和左右儿子AABB包围盒只有一个有相交则遍历对应儿子节点，如果和左右儿子AABB都相交先遍历交点近的儿子如果没有求得交点则遍历焦点远的儿子，如果有交点同样遍历焦点远的儿子，并选择左右交点较小的那个作为最终的结果。重复这个过程直到叶子节点，就转换为了场景元素(如这里的三角形)与光线的求交问题。

```c++
bool Hit(Ray& r, float& t_min, float& t_max, HitRecord& rec){
    //判断与左右儿子包围盒是否相交
    bool is_left_hit = child[0]->GetBoundingBox()->HitTest(r, t_min, t_max, rec_left);
    bool is_right_hit = child[1]->GetBoundingBox()->HitTest(r, t_min, t_max, rec_left);
    
    //如果都相交
    if(is_left_hit&&is_right_hit){
        //选择远近交点
        int near, far;
		if (rec_left < rec_right) near = 0;
		else near = 1;
        far = 1 - near;
        
        //如果近相交
        if (child[near]->hit(r, t_min, t_max, rec)) {
			if (rec.t < t_hit[far])
				return true;
            //远儿子可能会有更近的三角形相交点
			HitRecord tp_far_rec;
			if (child[far]->hit(r, t_min, t_max, tp_far_rec) && tp_far_rec.t < rec.t) {
				rec = tp_far_rec;
			}
			return true;
		}
        //如果近的不相交
		else
		{
			return child[far]->hit(r, t_min, t_max, rec);
		}
    }
    
    //如果只有一个相交
    if(is_left_hit) return child[0]->hit(r, t_min, t_max, rec);
    if(is_right_hit) return child[1]->hit(r, t_min, t_max, rec);
    
    //如果都不相交
    return false;
}
```

- 除了构建和遍历之外还有两个核心要点也就是光线与AABB包围盒的求交和与三角形的求交。与AABB的求交思路是计算光线与各个轴的交点k，取每个轴中k最大和最小的k，如果所有轴中最小值的最大值小于所有轴中最大值的最小值则说明则说明光线与包围盒相交，交点即为最小值的最大值。而三角形与光线的求交则可以参考wiki的代码实现（https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm），基本思路是列出$O+tD=(1-u-v)A+uB+vC$ 写成矩阵形式用克莱姆法则进行求解，在此之外检测光线是否平行于三角形以及交点是否落在三角形之外。

（4）我们从相机出发对着每个像素值射出一条光线(由于像素本身有大小所有每次射出光线可以在该像素位置上下、左右$[-0.5,0.5]$)内偏移，并通过一系列计算得到了最终的颜色值，我们只需要将累加的颜色值除以总的spp数量，并进行适当的gamma校正即可得到最终的结果，对应实现在ImageBuffer中，并通过stbi_write_jpg将其输出为jpg格式的图片，当然也可以类似《raytracing in one weekend》的方法输出成PPM格式图片。



## 三、渲染主循环和采样方法

下面的伪代码给出了我们对于每个像素采样和渲染的循环，而循环的具体实现和相关技术在后续进行了详细地阐述和分析。

```c++
for(int bn=0; ; bn++){
    //是否与场景相交
    if(!bvh_tree->hit(r, min_hit_value, max_hit_value, rec)) break;
    
    //（1）如果与光源相交
    if(rec.mtl->mtl_type=="Light"){
        if(is_first_Intersect_light && dot(rec.n, r.dir)<0){
            color = color + factor * (rec.mtl)->Ke;
        }
        break;
    }
    
    //上一次光的出射方向和这次光的入射方向相反
    wo = -r.dir;
    
    //（2）如果与玻璃材质相交
    if(rec.mtl->mtl_type=="Glass"){
        rec.mtl->scatter(wo, wi, rec);
        factor = factor*rec.mtl->bsdf(wo, wi,rec);
        r = Ray(rec.p, wi);
        continue;
    }
    
    //防止对于某点采样两次直接光照的flag
    is_first_Intersect_light = false;
    
    if(dot(rec.n, wo)<0) rec.n = -rec.n;
    //（3）多重重要性采样：采样光源+采样材质方向
    //先采样光源
    float light_pdf = light_list.SampleRay(rec, light_rec, bvh_tree, wi);
    if(light_pdf > epsilon){
        //得到材质对应的pdf
        bsdf_pdf = rec.mtl->pdf(wo, wi, rec);
        if(bsdf_pdf > epsilon){
            //得到对应的bsdf系数，保持物理上能量守恒
            bsdf_pdf_factor = rec.mtl->bsdf(wo, wi, rec);
            //用平方权重加权
            weight=light_pdf * light_pdf / (light_pdf * light_pdf + bsdf_pdf * bsdf_pdf);
			color += weight * (light_rec.mtl)->Ke * bsdf_pdf_factor * dot(wi, rec.n) / light_pdf;
        }
    }
    
    //再采样材质出射方向(用于更好地收敛高光项)
    bsdf_pdf = rec.mtl->scatter(wo, wi, rec);
    if (bsdf_pdf > epsilon) {
        Ray r(rec.p, wi);
        //计算采样到对应光源的pdf
		light_pdf = light_list.pdf(light_rec, bvh_tree, r);
        if (light_pdf > epsilon) {
            //得到对应的bsdf系数，保持物理上能量守恒
            bsdf_pdf_factor = rec.mtl->bsdf(wo, wi, rec);
            //同样平方加权
            weight = bsdf_pdf * bsdf_pdf / (light_pdf * light_pdf + bsdf_pdf * bsdf_pdf);
            color += weight * (light_rec.mtl)->Ke * bsdf_pdf_factor * dot(wi, rec.n) / bsdf_pdf;
        }
    }
    
    //（4）俄罗斯轮盘赌
    if(bn>=3){
        int which = 0;
        for(int i=1; i<3; i++){
            if(factor[i]>factor[which]) which = i;
        }
        if(get_uniform_random() < factor[which]) factor = factor/factor[which];
        else break;
    }
}
```

（1）首先需要注意的是由于我们每次打到非光源物体都会对光源采样，这就意味着这次采样得到的光源是对于这点的直接光照光源，如果进入下一次bounce并且反弹过后的射线正好打到光源那么我们将终止循环且不计算该光源的贡献，不然的话对于该点来说直接光源会被计算两次。

（2）与玻璃材质相交时的处理较为简单，光线会变为折射光线或者反射光线，由于光路确定没有进行采样所以此时的pdf值为1。我们利用如下Schlick公式近似菲涅尔项，并随机一个$[0,1]$的数，如果小于该项则出射光线为反射大于该项则为折射(同时还需要考虑全反射的情况)。当光线反射时对应的bsdf值为1，而当光线折射时对应的bsdf值为玻璃材质的transmission项用于能量的衰减。
$$
R(\theta)=R_0+(1-R_0)(1-cos\theta)^5,R_0=(\frac{\eta_i-\eta_t}{\eta_i+\eta_t})^2
$$
（3）通过多重重要性采样的方式可以更快地收敛高光项和diffuse项，其中的关键包括以下几个部分（为了统一wi指代的是交点与光源连接的方向，wo指代的是交点与物体连接的方向）。

- 光源的采样：我们首先可以根据面积选择应该采样哪一个光源，面积越大采样到该光源的概率也就越大即$pdf_1=\frac{A_i}{A_{all}}$ 选择了该光源后由于使用的光源均为三角形面片，可以通过如下方式在三角形上均匀随机采样一个点($a_1,a_2$)是两个$[0,1]$之间的随机数，而采样到该点的概率可以写作该三角形近似所占立体角分之一，即对应的立体角是$\frac{A_icos\theta}{R^2}$ 这里$\theta$ 是采样光源点和交点连线与三角形法线的交点，因此采到该点的概率是其取倒数$pdf_2=\frac{R^2}{A_icos\theta}$ ，两者相乘即为采到该光源的 $pdf=\frac{R^2}{A_{all}cos\theta}$ .

$$
p[0]*(1-\sqrt{a_1})+p[1]*(\sqrt{a_1}*(1-a_2))+p[2]*(\sqrt{a_1}*a_2)
$$

- 材质采样：
  - 对于diffuse材质而言，由于Phong模型中会乘以一个光源方向和法线的cos项，因而采样的时候我们也按照半球上cos系数的采样方式，此时的pdf为 $\frac{cos\theta}{\pi}$，采样方式是局部坐标系内 $\phi=2\pi\epsilon_1,\theta=arcos\sqrt{1-\epsilon_2}$ 
  
  ```c++
  float cos_phi = cosf(get_uniform_random(0, 2 * Pi));
  float cos_theta = sqrtf(1.0f - get_uniform_random());
  float sin_phi = sqrtf(1.0f - cos_phi * cos_phi);
  float sin_theta = sqrtf(1.0f - cos_theta * cos_theta);
  vec3 p_local(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
  
  wi = local2world(p_local, rec.n);
  pdf = cos_theta / Pi;
  
  //这里从一个随机的法线(已知法线，再随机一个垂直向量，由这两个向量计算第三个向量)坐标系转换为世界坐标系
  vec3 local2world(vec3& p_local, vec3& axis_z) {
  	vec3 axis_x, axis_y;
  	do {
  		axis_y = vec3(get_uniform_random(-1, 1), get_uniform_random(-1, 1), get_uniform_random(-1, 1));
  	} while (glm::length(cross(axis_y, axis_z)) < 0.0000001);
  	axis_y = normalize(cross(axis_y, axis_z));
  	axis_x = normalize(cross(axis_y, axis_z));
  	mat3 l2w = mat3(axis_x, axis_y, axis_z);
  	return l2w * p_local;
  }
  ```
  
  - 对于specular材质而言，这里需要考虑使用的是Phong模型(https://www.cs.princeton.edu/courses/archive/fall16/cos526/papers/importance.pdf)还是Bling Phong模型(http://farbrausch.de/~fg/index.html)前者按照光线真实视线方向和实际视线方向的夹角和高光系数Ns采样，后者按照半角向量与法线的夹角和高光系数Ns采样。采样方式是局部坐标系内$\phi=2\pi\epsilon_1,\theta=arcos(1-\epsilon_2)^{\frac{1}{Ns+2}}$  
  
  ```c++
  float cos_phi = cosf(get_uniform_random(0, 2 * Pi));
  float cos_theta = powf(1 - get_uniform_random(), 1.0f / (Ns + 2));
  float sin_phi = sqrtf(1.0f - cos_phi * cos_phi);
  float sin_theta = sqrtf(1.0f - cos_theta * cos_theta);
  vec3 p_local(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
  
  //Phong模型
  if (PhongOrBling == 0) {
      vec3 true_wi = 2 * dot(wo, rec.n) * rec.n - wo;
      wi = local2world(p_local, true_wi);
      pdf = (Ns + 1) / (2 * Pi) * powf(cos_theta, Ns);
  }
  //Bling Phong模型
  else
  {
      vec3 half_vector = local2world(p_local, rec.n);
      float cos_half_vector = dot(half_vector, wo);
      wi = 2.0f * cos_half_vector * half_vector - wo;
      pdf = (Ns + 2) / (2 * Pi) * powf(cos_theta, Ns+1);
      pdf = pdf / (4 * cos_half_vector);
  }
  return pdf;
  ```

- 材质的bsdf：为了保证phong/Bling-Phong模型的物理能量守恒性，还需要乘以一个bsdf系数，对于diffuse项这个系数是一个常量 $K_d/\pi$ 而对于specular项根据使用的Phong模型和Bling Phong模型的不同，按照上述参考资料分别表示为 $0.5Ks(Ns+2)(dot(w_{otrue},w_o))^{Ns}$ 以及$\frac{(Ns+2)(Ns+4)}{8\pi(2^{-Ns/2}+Ns)}dot(n_{halfvector},n)^{Ns}$ 。需要注意的是可以使用Bling-Phong模型计算bsdf而用Phong模型进行采样，因为采样的方式只会影响收敛的速度并不会影响结果(也就是均值不变，方差变化)。

```c++
vec3 res_kd = Kd;
if (is_texture) res_kd = texture->Get(rec.uv);
vec3 factor;

//Phong模型
if(PhongOrBling==0){
    vec3 true_wo = 2 * dot(wi, rec.n) * rec.n - wi;
    factor = res_kd + 0.5f * Ks * (Ns + 2) * powf(dot(true_wo, wo), Ns);
    factor = factor / Pi;
}
//Bling Phong模型
else
{
    vec3 half_vector = normalize(wo + wi);
    factor = res_kd + 0.125f * Ks * (Ns + 2) * (Ns + 4)  * powf(dot(half_vector, rec.n), Ns) / (Ns + powf(2, -Ns / 2));
    factor = factor / Pi;
}
return factor;
```

（4）为了保证能量的不损失而且光线不会一直弹射下去，可以通过俄罗斯轮盘赌的方式在每次光线弹射后根据当前的能量系数factor决定光线是否下次的弹射，如果继续弹射那么对应的系数factor = factor/p这样可以保持最终计算结果的期望与无限次弹射的期望值相同，从而保证了能量的守恒性。



## 四、结果展示

（1）cornell-box：用于显示diffuse材质的效果

<img src=".\img\cornell-box\cornell-box_spp4.jpg" alt="cornell-box_spp4" style="zoom:100%;" />

【cornell-box 4spp】



<img src=".\img\cornell-box\cornell-box_spp32.jpg" alt="cornell-box_spp32" style="zoom:100%;" />

【cornell-box 32spp】



<img src=".\img\cornell-box\cornell-box_spp256.jpg" alt="cornell-box_spp256" style="zoom:100%;" />

【cornell-box 256spp】



<img src=".\img\cornell-box\cornell-box_spp4096.jpg" alt="cornell-box_spp4096" style="zoom:100%;" />

【cornell-box 4096spp】



（2）staircase：用于展示glass材质+diffuse材质

<img src=".\img\stairscase\stairscase_spp4.jpg" alt="stairscase_spp4" style="zoom:100%;" />

【staircase 4spp】



<img src=".\img\stairscase\stairscase_spp32.jpg" alt="stairscase_spp32" style="zoom:100%;" />

【staircase 32spp】



<img src=".\img\stairscase\stairscase_spp256.jpg" alt="stairscase_spp256" style="zoom:100%;" />

【staircase 256spp】



<img src=".\img\stairscase\stairscase_spp4096.jpg" alt="stairscase_spp4096" style="zoom:100%;" />

【staircase 4096spp】



（3）veach-mis：用于展示diffuse+specular材质

<img src=".\img\veach-mis\veach-mis_spp4.jpg" alt="veach-mis_spp4" style="zoom:100%;" />

【veach-mis 4spp】



<img src=".\img\veach-mis\veach-mis_spp32.jpg" alt="veach-mis_spp32" style="zoom:100%;" />

【veach-mis 32spp】



<img src=".\img\veach-mis\veach-mis_spp256.jpg" alt="veach-mis_spp256" style="zoom:100%;" />

【veach-mis 256spp】



<img src=".\img\veach-mis\veach-mis_spp4096.jpg" alt="veach-mis_spp4096" style="zoom:100%;" />

【veach-mis 4096spp】



（4）living-room：其他大场景

<img src=".\img\LivingRoom\living_room_spp4.jpg" alt="living_room_spp4" style="zoom:100%;" />

【living-room 4spp】



<img src=".\img\LivingRoom\living_room_spp32.jpg" alt="living_room_spp32" style="zoom:100%;" />

【living-room 32spp】



<img src=".\img\LivingRoom\living_room_spp256.jpg" alt="living_room_spp256" style="zoom:100%;" />

【living-room 256spp】



<img src=".\img\LivingRoom\living_room_spp2048.jpg" alt="living_room_spp2048" style="zoom:100%;" />

【living-room 2048spp】


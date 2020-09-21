{
    'headers': [
        'akari/kernel/soa.h',
        'akari/kernel/integrators/gpu/workitem.h'
    ],
    'flat': ['int', 'bool', 'Float', 'Array3f',
             'Array2f', 'Sampler<C>', 'Ray<C>', 'Spectrum',
             'Intersection<C>', 'const Material<C> *'],
    'soa': {
        'PathState<C>': {
            'template': 'C',
            'fields': {
                'sampler': 'Sampler<C>',
                'L': 'Spectrum',
                'beta': 'Spectrum',
                # 'terminated': 'bool'
            }
        },
        'MaterialWorkItem<C>': {
            'template': 'C',
            'fields': {
                'pixel':'int',
                'material': 'const Material<C> *',
                'geom_id': 'int',
                'prim_id': 'int',
                'uv': 'Array2f',
                'wo':'Array3f'
            }
        },
        #  'CameraRayWorkItem<C>':{
        #     'template':'C',
        #     'fields':{
        #         'sample':'CameraSample<C>'
        #     }
        # },
        'RayWorkItem<C>': {
            'template': 'C',
            'fields': {
                'pixel': 'int',
                'ray': 'Ray<C>',

            }
        },
        # 'ShadowRayWorkItem<C>':{
        #     'template':'C',
        #     'fields':{
        #         'pixel':'int',
        #         'ray':'Ray<C>'
        #     }
        # },
        # 'ClosestHitWorkItem<C>':{
        #     'template':'C',
        #     'fields':{
        #         'pixel':'int',
        #         'intersection':'Intersection<C>'
        #     }
        # },
        # 'AnyHitWorkItem<C>':{
        #     'template':'C',
        #     'fields':{
        #         'pixel':'int',
        #         'hit':'bool'
        #     }
        # },
        #  'MissWorkItem<C>':{
        #     'template':'C',
        #     'fields':{
        #         'pixel':'int'
        #     }
        # }
    }
}
